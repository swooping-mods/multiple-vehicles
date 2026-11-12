#ifndef MOON_MULTIPLE_VEHICLE_PURCHASE_H
#define MOON_MULTIPLE_VEHICLE_PURCHASE_H
/* Included after the local native/fleet adapters. The native transaction owns
 * debit and compensation; an ambiguous result must never invite another debit. */
static struct {
 DRoot world,buyer,key,record;
 MoonFleetSnapshot prior;
 DRoot prior_storage[MOON_FLEET_LIMIT];
 int active,native,garage,model,home,lookup_seen,assigned,hook_failed,uncertain;
 int next_home;
} mv={.next_home=-1000000};

static void mv_close(void){
 mv.native=0;mv.active=0;
 ddrop(&mv.record);ddrop(&mv.key);ddrop(&mv.buyer);ddrop(&mv.world);
 for(unsigned i=0;i<mv.prior.count;i++)ddrop(&mv.prior_storage[i]);
 mf_close(&mv.prior);
 mv.home=0;mv.lookup_seen=0;mv.assigned=0;mv.hook_failed=0;
}
static int mv_same(PTR a,PTR b,int *ok){
 if(!a||!b){*ok=0;return 0;}
 void *args[]={a,b};PTR box=call(mf.equal,0,args,ok);
 PTR answer=box?unbox(box):0;if(!answer){*ok=0;return 0;}
 return *ok&&*(unsigned char *)answer;
}
static int mv_hook(int id,Context *c){
 /* Check the thread before reading the transaction's rooted managed state. */
 if(!thread_id||GetCurrentThreadId()!=thread_id||!mv.native||!c->rsp)return 0;
 U64 site=*(U64 *)c->rsp;
 if(id==1){
  if(site!=(U64)(buy_address+0x234)||(PTR)c->rcx!=dget(mv.world))return 0;
  if(mv.lookup_seen||mv.assigned||mv.hook_failed){mv.hook_failed=1;return 0;}
  mv.lookup_seen=1;c->rax=0;return 1;
 }
 if(id!=2||site!=(U64)(buy_address+0x3c8))return 0;
 if(!mv.lookup_seen||mv.assigned||mv.hook_failed||!mv.home)goto reject;
 PTR owned=(PTR)c->rcx;
 if(!owned||object_class(owned)!=owned_class)goto reject;
 /* Never rewrite a record that existed before the native purchase. */
 for(unsigned i=0;i<mv.prior.count;i++)if(mf_item(&mv.prior,i)==owned)goto reject;
 mv.record=dhold(owned);if(!mv.record)goto reject;
 int garage=0,model=-1;PTR owner=0;
 field_value(dget(mv.record),owned_garage_f,&garage);
 field_value(dget(mv.record),mf.model,&model);
 field_value(dget(mv.record),mf.owner,&owner);
 int ok=1;
 if(garage!=mv.garage||model!=mv.model||!mv_same(owner,dget(mv.buyer),&ok)||!ok)goto reject;
 set_field(dget(mv.record),owned_garage_f,&mv.home);
 field_value(dget(mv.record),owned_garage_f,&garage);
 if(garage!=mv.home)goto reject;
 mv.assigned=1;event("distinct-vehicle-key-assigned");return 0;
reject:
 /* The existing adapter's null-storage refusal prevents a mismatched record
  * from silently receiving an ordinary garage key. Native rollback remains
  * unqualified; the uncertainty latch prevents another purchase. */
 mv.hook_failed=1;c->rax=0;return 1;
}
static int mv_reserve(void){
 for(int tries=0;tries<4096;tries++){
  int home=mv.next_home;
  mv.next_home=home<=-2000000000?-1000000:home-1;
  int ok=1;void *args[]={&home,&mv.model,dget(mv.buyer)};
  PTR key=call(key_m,0,args,&ok);if(!ok||!key)return 0;
  mv.key=dhold(key);if(!mv.key)return 0;
  PTR exists=mf_lookup(dget(mv.world),dget(mv.key),&ok);
  if(!ok)return 0;
  int collision=exists!=0;
  for(unsigned i=0;!collision&&i<mv.prior.count;i++){
   collision=mv_same(dget(mv.prior_storage[i]),dget(mv.key),&ok);
   if(!ok)return 0;
  }
  if(!collision){mv.home=home;return 1;}
  ddrop(&mv.key);
 }
 event("vehicle-key-limit");return 0;
}
static int mv_confirm(void){
 if(!mv.lookup_seen||!mv.assigned||mv.hook_failed||!dget(mv.record))return 0;
 MoonFleetRecord value={0};if(!mf_read(dget(mv.record),&value))return 0;
 DRoot storage=dhold(value.storage),owner=dhold(value.owner);
 int ok=1;int matches=storage&&owner&&value.garage==mv.home&&value.model==mv.model&&
  mv_same(dget(storage),dget(mv.key),&ok)&&mv_same(dget(owner),dget(mv.buyer),&ok)&&ok;
 ddrop(&storage);ddrop(&owner);if(!matches)return 0;
 if(mf_lookup(dget(mv.world),dget(mv.key),&ok)!=dget(mv.record)||!ok)return 0;
 MoonFleetSnapshot after={0};if(!mf_snapshot(dget(mv.world),&after))return 0;
 matches=after.count==mv.prior.count+1;
 for(unsigned i=0;matches&&i<mv.prior.count;i++){
  unsigned count=0;for(unsigned j=0;j<after.count;j++)count+=mf_item(&mv.prior,i)==mf_item(&after,j);
  if(count!=1)matches=0;
  MoonFleetRecord prior={0};
  if(matches&&(!mf_read(mf_item(&mv.prior,i),&prior)||
   !mv_same(prior.storage,dget(mv.prior_storage[i]),&ok)||!ok))matches=0;
 }
 unsigned added=0;for(unsigned i=0;i<after.count;i++)added+=mf_item(&after,i)==dget(mv.record);
 mf_close(&after);return matches&&added==1;
}
static int mv_purchase(PTR world,PTR buyer,int garage,int model){
 if(mv.active||mv.uncertain||garage<0||model<0)return 0;
 mv.active=1;mv.garage=garage;mv.model=model;
 mv.world=dhold(world);mv.buyer=dhold(buyer);
 if(!mv.world||!mv.buyer||!mf_snapshot(dget(mv.world),&mv.prior))goto refused;
 /* Leave room for the required post-transaction snapshot. */
 if(mv.prior.count>=MOON_FLEET_LIMIT){event("vehicle-fleet-limit");goto refused;}
 for(unsigned i=0;i<mv.prior.count;i++){
  MoonFleetRecord prior={0};
  if(!mf_read(mf_item(&mv.prior,i),&prior))goto refused;
  mv.prior_storage[i]=dhold(prior.storage);if(!mv.prior_storage[i])goto refused;
 }
 if(!mv_reserve())goto refused;
 PTR message=0;int ok=1;void *args[]={dget(mv.buyer),&garage,&model,&message};
 mv.native=1;PTR result=call(buy_m,dget(mv.world),args,&ok);mv.native=0;
 PTR answer=result?unbox(result):0;int bought=answer&&*(unsigned char *)answer;
 if(!ok||!answer||mv.hook_failed||(!bought&&mv.assigned))goto uncertain;
 if(!bought)goto refused;
 if(!mv_confirm())goto uncertain;
 call(save_m,dget(mv.world),0,&ok);if(!ok)goto uncertain;
 /* RequestSave returning is a dispatch observation, not durable-save evidence. */
 event("additional-vehicle-purchase-save-requested");mv_close();return 1;
uncertain:
 mv.uncertain=1;event("additional-vehicle-purchase-uncertain");mv_close();return 0;
refused:
 event("additional-vehicle-purchase-refused");mv_close();return 0;
}
#endif

/* Independent repeated purchase path; virtual home IDs preserve distinct native keys. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
static PTR buy_m,key_m,save_m,menu_visible_m,garage_f,selected_f,owned_garage_f;
static PTR purchase_world;static int purchasing,reserved_identity,next_identity=-1000000;static unsigned char *buy_address;
static int before(int id,Context *c){
 if(id==1){/* Only the duplicate lookup within our synchronous native purchase. */
  if(purchasing&&(PTR)c->rcx==purchase_world&&*(U64*)c->rsp==(U64)(buy_address+0x234)){c->rax=0;return 1;}return 0;
 }
 if(id==2){
  if(!purchasing||*(U64*)c->rsp!=(U64)(buy_address+0x3c8))return 0;
  if(!reserved_identity){c->rax=0;return 1;}PTR owned=(PTR)c->rcx;set_field(owned,owned_garage_f,&reserved_identity);event("distinct-vehicle-key-assigned");return 0;
 }
 if(!on_thread())return 0;int edge=moon_input_action("multiple-vehicles.purchase","Alt+N",1).pressed,ok=1;PTR walker=(PTR)c->rcx,world=0;field_value(walker,world_f,&world);DWORD pid=0;window_pid(foreground(),&pid);
 if(!edge||!alive(world)||!host()||pid!=GetCurrentProcessId()||!boolean(focus_m,0,&ok)||boolean(typing_m,0,&ok)||boolean(restoring_m,world,&ok)||!ok)return 0;
 PTR menu=component(walker,"MoonGarageMenu",&ok);if(!menu||!boolean(menu_visible_m,menu,&ok)||!ok)return 0;PTR selected=0;int garage=0,index=-1;field_value(menu,garage_f,&garage);field_value(menu,selected_f,&selected);if(!selected)return 0;PTR vf=get_field(object_class(selected),"vehicle");if(!vf)return 0;field_value(selected,vf,&index);PTR player=call(dmethod("MoonCleaners","MoonCrew","get_LocalPlayerId",""),0,0,&ok),message=0;if(!ok||!player||garage<0||index<0)return 0;
 /* Reserve a collision-free native key before charging any money. */
 reserved_identity=0;for(int tries=0;tries<4096;tries++){int home=next_identity--;if(next_identity< -2000000000)next_identity=-1000000;void *ka[]={&home,&index,player};PTR key_obj=call(key_m,0,ka,&ok);PTR exists=key_obj?mf_lookup(world,key_obj,&ok):0;if(!ok||!key_obj)return 0;if(!exists){reserved_identity=home;break;}}if(!reserved_identity){event("vehicle-key-limit");return 0;}
 purchase_world=world;purchasing=1;void *a[]={player,&garage,&index,&message};PTR bought=call(buy_m,world,a,&ok);purchasing=0;purchase_world=0;reserved_identity=0;if(ok&&bought&&*(unsigned char*)unbox(bought)){call(save_m,world,0,&ok);event("additional-vehicle-purchased");}else event("additional-vehicle-purchase-refused");return 0;
}
int MoonLab_ModStart(PTR domain,PTR game){if(!common(domain,game,"multiple-vehicles.jsonl")||!mf_bind(game,0))return 0;
(void)pressed;
#define M(F,T,N,S) F=dmethod("MoonCleaners",T,N,S);if(!F)return 0
 M(buy_m,"MoonWorld","TryBuyVehicle","System.String;System.Int32;System.Int32;System.String&");M(key_m,"MoonWorld","GarageVehicleStorage","System.Int32;System.Int32;System.String");M(save_m,"MoonWorld","RequestSave","");M(menu_visible_m,"MoonGarageMenu","get_IsOpen","");
#undef M
 PTR menu=find_class("MoonCleaners","MoonGarageMenu");garage_f=get_field(menu,"garage");selected_f=get_field(menu,"selected");owned_garage_f=get_field(find_class("MoonCleaners","MoonOwnedVehicle"),"garage");if(!garage_f||!selected_f||!owned_garage_f||method_token(buy_m)!=100665188)return 0;
 buy_address=*(unsigned char**)buy_m;static const unsigned char purchase_shape[]={0x48,0x89,0x5c,0x24,0x18};for(int i=0;i<5;i++)if(buy_address[i]!=purchase_shape[i])return 0;
 /* Pin both callsites used to scope the hooks to this exact purchase body. */
 if(buy_address[0x22f]!=0xe8||buy_address[0x3c3]!=0xe8)return 0;
 PTR lookup=dmethod("MoonCleaners","MoonWorld","OwnedVehicle","System.Int32;System.Int32;System.String"),storage=dmethod("MoonCleaners","MoonOwnedVehicle","get_Storage","");if(!lookup||!storage||buy_address+0x234+*(int*)(buy_address+0x230)!=*(unsigned char**)lookup||buy_address+0x3c8+*(int*)(buy_address+0x3c4)!=*(unsigned char**)storage)return 0;
 static const unsigned char lookup_shape[]={0x48,0x89,0x5c,0x24,0x08},storage_shape[]={0x48,0x89,0x5c,0x24,0x20};
 if(!update_hook(game)||!exact_hook(dmethod("MoonCleaners","MoonWorld","OwnedVehicle","System.Int32;System.Int32;System.String"),100665387,1,game,lookup_shape,5)||!exact_hook(dmethod("MoonCleaners","MoonOwnedVehicle","get_Storage",""),100668561,2,game,storage_shape,5))return 0;before_hook=before;return install_hooks();}

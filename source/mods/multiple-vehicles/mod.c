/* Independent repeated purchase path; virtual home IDs preserve distinct native keys. */
#include "base.h"
#include "../../runtime/native/input.h"
#include "../../runtime/native/local_actor.h"
#include "../vehicle-framework/sdk/fleet_adapter.h"
static PTR buy_m,key_m,save_m,menu_visible_m,garage_f,selected_f,owned_garage_f,owned_class;
static unsigned char *buy_address;
#include "purchase.h"
#include "frame_admission.h"
static int before(int id,Context *c){
 if(id==1||id==2)return mv_hook(id,c);
 if(id!=0||!on_thread()||mv.active||mv.uncertain)return 0;
 PTR walker=(PTR)c->rcx,world=0,player=0;int ok=1,garage=-1,index=-1;
 if(!mv_local_admit(walker))return 0;
 field_value(walker,world_f,&world);DWORD pid=0;window_pid(foreground(),&pid);
 int ready=alive(world)&&host()&&pid==GetCurrentProcessId()&&vehicle_read_boolean(focus_m,0,&ok)&&
  !vehicle_read_boolean(typing_m,0,&ok)&&!vehicle_read_boolean(restoring_m,world,&ok)&&ok;
 PTR menu=ready?component(walker,"MoonGarageMenu",&ok):0;
 ready=ready&&menu&&vehicle_read_boolean(menu_visible_m,menu,&ok)&&ok;
 if(ready){
  PTR selected=0;field_value(menu,garage_f,&garage);field_value(menu,selected_f,&selected);
  PTR vf=selected?get_field(object_class(selected),"vehicle"):0;
  if(vf)field_value(selected,vf,&index);
  player=call(mf.local,0,0,&ok);ready=ok&&player&&garage>=0&&index>=0&&vf;
 }
 int edge=moon_input_action("multiple-vehicles.purchase","Alt+N",ready).pressed;
 if(ready&&edge)mv_purchase(world,player,garage,index);return 0;
}
int MoonLab_ModStart(PTR domain,PTR game){if(!common(domain,game,"multiple-vehicles.jsonl")||!mf_bind(game,0)||!moon_local_actor_bind())return 0;
(void)pressed;
#define M(F,T,N,S) F=dmethod("MoonCleaners",T,N,S);if(!F)return 0
 M(buy_m,"MoonWorld","TryBuyVehicle","System.String;System.Int32;System.Int32;System.String&");M(key_m,"MoonWorld","GarageVehicleStorage","System.Int32;System.Int32;System.String");M(save_m,"MoonWorld","RequestSave","");M(menu_visible_m,"MoonGarageMenu","get_IsOpen","");
#undef M
 PTR menu=find_class("MoonCleaners","MoonGarageMenu");if(!menu)return 0;garage_f=get_field(menu,"garage");selected_f=get_field(menu,"selected");owned_class=find_class("MoonCleaners","MoonOwnedVehicle");if(!owned_class)return 0;owned_garage_f=get_field(owned_class,"garage");if(!garage_f||!selected_f||!owned_garage_f||method_token(buy_m)!=100665188)return 0;
 buy_address=*(unsigned char**)buy_m;if(!buy_address)return 0;static const unsigned char purchase_shape[]={0x48,0x89,0x5c,0x24,0x18};for(int i=0;i<5;i++)if(buy_address[i]!=purchase_shape[i])return 0;
 /* Pin both callsites used to scope the hooks to this exact purchase body. */
 if(buy_address[0x22f]!=0xe8||buy_address[0x3c3]!=0xe8)return 0;
 PTR lookup=dmethod("MoonCleaners","MoonWorld","OwnedVehicle","System.Int32;System.Int32;System.String"),storage=dmethod("MoonCleaners","MoonOwnedVehicle","get_Storage","");if(!lookup||!storage||buy_address+0x234+*(int*)(buy_address+0x230)!=*(unsigned char**)lookup||buy_address+0x3c8+*(int*)(buy_address+0x3c4)!=*(unsigned char**)storage)return 0;
 static const unsigned char lookup_shape[]={0x48,0x89,0x5c,0x24,0x08},storage_shape[]={0x48,0x89,0x5c,0x24,0x20};
 if(!update_hook(game)||!exact_hook(dmethod("MoonCleaners","MoonWorld","OwnedVehicle","System.Int32;System.Int32;System.String"),100665387,1,game,lookup_shape,5)||!exact_hook(dmethod("MoonCleaners","MoonOwnedVehicle","get_Storage",""),100668561,2,game,storage_shape,5))return 0;before_hook=before;return install_hooks();}

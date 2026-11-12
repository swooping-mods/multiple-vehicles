#ifndef MOON_MULTIPLE_FRAME_ADMISSION_H
#define MOON_MULTIPLE_FRAME_ADMISSION_H
static int mv_local_admit(PTR walker){
 PTR local=0;
 if(!moon_local_actor_read(&local)||!local){
  (void)moon_input_action("multiple-vehicles.purchase","Alt+N",0);return 0;
 }
 /* A peer callback cannot consume the local purchase edge or inspect its menu. */
 return walker==local;
}
#endif

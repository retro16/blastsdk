#ifndef SUB_IDLE_H
#define SUB_IDLE_H

static void sub_idle_init() {
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
  sync_main_sub();
#endif
}

static void sub_idle_vsync() {
#if TARGET == TARGET_SCD1 || TARGET == TARGET_SCD2
  sub_interrupt();
  bdp_sub_check();
#endif
}

#endif

#include "blsth.h"

struct blsth_frame {
  // Registers
  void *d2; void *d3; void *d4; void *d5; void *d6; void *d7;
  void *a2; void *a3; void *a4; void *frameptr; void *fp; void *retaddr;
};

struct th {
  char *stack;
};


// Global variables
volatile void *blsth_sp;
struct th *blsth_curthread;
void *mainstack;

void memcpy(char *d, const char *c, unsigned int s) {

  if(!((unsigned long)d & 1) && !((unsigned long)c & 1) && !(s & 1) && s < 0x20000)
  {

  asm(
"\n\t lsr.w #1, %2"
"\n\t subq #1, %2"
"\nmemcpy_fastcopy_loop:"
"\n\t move.w %0@+, %1@+"
"\n\t dbra %2, memcpy_fastcopy_loop"
  : "=a"(c), "=a"(d), "=d"(s)
  : "0"(c), "1"(d), "2"(s)
  : "0", "1", "2"
  );

  } else {
    while(s--) {
      *d = *c;
      ++(c);
      ++(d);
    }
  }
}

__attribute__ ((noinline)) void blsth_contextswitch(void *currentcontext, void *newcontext) {
  asm volatile("move.l %%sp, %0" :: "m"(currentcontext) : "%d2", "%d3", "%d4", "%d5", "%d6", "%d7", "%a2", "%a3", "%a4", "%a5", "%a6");
  asm volatile("move.l %0, %%sp" :: "m"(newcontext) : "%d2", "%d3", "%d4", "%d5", "%d6", "%d7", "%a2", "%a3", "%a4", "%a5", "%a6");
}

__attribute__ ((noinline)) void blsth_exitcontext(void *currentcontext, void *newcontext) {
  asm volatile("move.l %%sp, %0" :: "m"(currentcontext) : );
  asm volatile("move.l %0, %%sp" :: "m"(newcontext) : "%d2", "%d3", "%d4", "%d5", "%d6", "%d7", "%a2", "%a3", "%a4", "%a5", "%a6");
}

__attribute__ ((noinline)) void blsth_callfinished(void *thread) {
  // Remove parameters
  asm volatile("move.l %%a5, %%sp \n\t"
	       "move.l %%a5, %0"
	       :"=o" (blsth_curthread) :: );

  // Check if there is a new one
  if((char *)blsth_curthread->stack >= (char *)blsth_curthread)
  {
    // No more task : exit
    blsth_exitcontext(&blsth_curthread->stack, &mainstack);
  }

  // Remaining task : pop task state and resume
  asm volatile("movem.l %%sp@+, %%d2-%%d7/%%a2-%%a6" ::: );
}

__attribute__ ((noinline)) void blsth_pushcall(void *thread, void *fctptr, ...) {
  struct th *t = (struct th *)((void**)thread - 1);
  void *oldstack = t->stack;

  // Copy parameters except thread and fctptr
  register int framesize = (char*)(&fctptr) - (char*)blsth_sp - 4;
  t->stack -= framesize;
  memcpy(t->stack, (char*)fctptr + 4, framesize);

  t->stack -= sizeof(struct blsth_frame);
  struct blsth_frame *f = (struct blsth_frame *)t->stack;
  f->frameptr = oldstack;

  // Set return address to tail call blsth_callfinished
  f->retaddr = (char *)&blsth_callfinished;
}

__attribute__ ((noinline)) void blsth_yield() {
  blsth_contextswitch(&blsth_curthread->stack, &mainstack);
}

__attribute__ ((noinline)) void blsth_run(void *thread) {
  struct th *t = (struct th *)((void**)thread - 1);
  if((char *)t->stack < (char *)t) {
    // There are waiting jobs in the stack
    blsth_curthread = t;
    blsth_contextswitch(&mainstack, &t->stack);
  }
}

void blsth_init(void *thread) {
  struct th *t = (struct th *)((void**)thread - 1);
  t->stack = (char *)thread;
}


char threadstack[4096];
void *thread = &(threadstack[4096]);

void threaded_function(int a, int b) {
  int c;
  volatile int d = 0;
  for(c = a; c < b; ++c) {
    ++d;
    blsth_yield();
  }
}

int main() {
  blsth_init(thread);
  int x = 8, y;
  blsth_call(thread, threaded_function, 3, x); // Run second
  blsth_call(thread, threaded_function, 4, 7); // Run first

  for(y = 0; y < 5; ++y) {
    blsth_run(thread);
  }

  blsth_call(thread, threaded_function, 1, 10); // This function will interrupt the two others

  for(y = 0; y < 50; ++y) {
    blsth_run(thread); // Runs the third function, then finishes the second one and runs the first one.
  }

  return 0;
}


#ifndef BLS_TH_H
#define BLS_TH_H

/* Blast thread library.

This library allows to run blocking functions easily, with context
preservation. Calls are organized as a stack.

Sample :

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

void main() {
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
}

 */

/* Get stack pointer value */
#define BLS_SP_GET(varptr) asm volatile("move.l %%a7, %0": "=m"(varptr) ::)

/* Initialize a new thread stack

sp : the stack pointer used for the thread.
     This pointer should be passed to all calls.
*/
extern void blsth_init(void *sp);

/* Run waiting background tasks

The thread will run until all pending function calls are done or until the thread calls blsth_yield().
*/
__attribute__ ((noinline))
extern void blsth_run(void *sp);

/* Push a new call on thread stack */
#define blsth_call(sp, fct, ...) \
  do { BLS_SP_GET(blsth_sp); \
  blsth_pushcall(thread, fct, __VA_ARGS__); \
  asm volatile(""); \
  } while(0)

/* Called from a thread : release thread and give CPU back to main */
__attribute__ ((noinline))
extern void blsth_yield();

/* Used by blsth_call macro : DO NOT USE DIRECTLY */
__attribute__ ((noinline))
extern void blsth_pushcall(void *sp, void *fctptr, ...);

/* Used by blsth_call macro : DO NOT USE DIRECTLY */
extern volatile void *blsth_sp;

#endif

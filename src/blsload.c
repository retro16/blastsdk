struct blsth_frame {
  // Registers
  void *d2; void *d3; void *d4; void *d5; void *d6; void *d7;
  void *a2; void *a3; void *a4; void *frameptr; void *fp; void *retaddr;
};

struct th {
  char *stack;
};

void blsfastfill_word(char *d, unsigned short count, unsigned short pattern)
{
  asm(
"\n\t subq #1, %1"
"\n1:"
"\n\t move.w %2, %0@+"
"\n\t dbra %1, 1b"
  :
  : "a"(d), "d"(count), "d"(pattern)
  : "0", "1"
  );
}

void blsfastcopy_word(char *d, const char *c, unsigned int s) {
  asm(
"\n\t subq #1, %2"
"\n1:"
"\n\t move.w %0@+, %1@+"
"\n\t dbra %2, 1b"
  : 
  : "a"(c), "a"(d), "d"(s)
  : "0", "1", "2"
  );
}

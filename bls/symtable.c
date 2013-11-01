#include "bls.h"
#include <string.h>
#include <stdlib.h>

typedef struct dsym {
  struct dsym *next;
  char name[NAMELEN];
  u32 val;
} *dsymptr;

dsymptr dsymtable = 0;

void setdsym(const char *name, u32 val)
{
  dsymptr s;
  for(s = dsymtable; s; s = s->next)
  {
    if(strcmp(name, s->name) == 0)
    {
      strcpy(s->name, name);
      s->val = val;
      return;
    }
  }

  s = malloc(sizeof(*s));
  strcpy(s->name, name);
  s->val = val;
  s->next = dsymtable;
  dsymtable = s;
}
/*
u32 * getdsym(const char *name)
{
  // TODO
}
*/

const char * getdsymat(u32 val)
{
  dsymptr s;
  for(s = dsymtable; s; s = s->next)
  {
    if(s->val == val)
    {
      return s->name;
    }
  }

  return 0;
}
/*
int parsedsym(const char *text)
{
}

int parsedsymfile(const char *name)
{
}
*/

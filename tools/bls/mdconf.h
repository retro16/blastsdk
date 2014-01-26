#ifndef MDCONF_H
#define MDCONF_H

typedef struct mdconfnode {
  char *key;
  char *value;
  struct mdconfnode *prev;
  struct mdconfnode *next;
  struct mdconfnode *child;
  struct mdconfnode *parent;
} mdconfnode;

mdconfnode * mdconfappendsibling(mdconfnode *node);
mdconfnode * mdconfappendchild(mdconfnode *node);
mdconfnode * mdconfcopy(const mdconfnode *node);
void mdconffree(mdconfnode *root);
mdconfnode * mdconfparsefile(const char *filename);
const mdconfnode * mdconfsearch(const mdconfnode *node, const char *path);
const char *mdconfget(const mdconfnode *root, const char *path);
const char *mdconfgetvalue(const mdconfnode *node, const char *path);
void mdconfprint(const mdconfnode *root, int depth);

#endif//MDCONF_H

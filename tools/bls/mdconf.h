#ifndef MDCONF_H
#define MDCONF_H

typedef struct mdconfnode {
  char *key;
  char *value;
  int title;
  struct mdconfnode *prev;
  struct mdconfnode *next;
  struct mdconfnode *child;
  struct mdconfnode *parent;
} mdconfnode;

extern char *(*mdconf_prefixes)[4096];

mdconfnode *mdconfcreatesibling(mdconfnode *node);
void mdconfappendsibling(mdconfnode *node, mdconfnode *sibling);
mdconfnode *mdconfcreatechild(mdconfnode *node);
void mdconfappendchild(mdconfnode *node, mdconfnode *child);
mdconfnode *mdconfcopy(const mdconfnode *node);
void mdconffree(mdconfnode *root);
mdconfnode *mdconfremove(mdconfnode *node);
mdconfnode *mdconfparsefile(const char *filename);
const mdconfnode *mdconfsearch(const mdconfnode *node, const char *path);
const char *mdconfget(const mdconfnode *root, const char *path);
const char *mdconfgetvalue(const mdconfnode *node, const char *path);
void mdconfprint(const mdconfnode *root, int depth);

#endif//MDCONF_H

// vim: ts=2 sw=2 sts=2 et

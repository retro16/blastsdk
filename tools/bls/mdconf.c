#include "mdconf.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

mdconfnode *mdconfcreatesibling(mdconfnode *node)
{
  if(!node) {
    return NULL;
  }

  while(node->next) {
    node = node->next;
  }

  node->next = (mdconfnode *)malloc(sizeof(mdconfnode));
  node->next->key = NULL;
  node->next->value = NULL;
  node->next->title = -1;
  node->next->prev = node;
  node->next->next = NULL;
  node->next->child = NULL;
  node->next->parent = node->parent;
  return node->next;
}

void mdconfappendsibling(mdconfnode *node, mdconfnode *sibling)
{
  if(!node) {
    return;
  }

  while(node->next) {
    node = node->next;
  }

  mdconfremove(sibling); // Detach sibling from existing tree

  node->next = sibling;
  node->next->prev = node;
  node->next->parent = node->parent;
}

mdconfnode *mdconfcreatechild(mdconfnode *node)
{
  if(!node) {
    return NULL;
  }

  if(node->child) {
    return mdconfcreatesibling(node->child);
  }

  node->child = (mdconfnode *)malloc(sizeof(mdconfnode));
  node->child->key = NULL;
  node->child->value = NULL;
  node->child->title = -1;
  node->child->prev = NULL;
  node->child->next = NULL;
  node->child->child = NULL;
  node->child->parent = node;
  return node->child;
}

void mdconfappendchild(mdconfnode *node, mdconfnode *child)
{
  if(!node) {
    return;
  }

  if(node->child) {
    mdconfappendsibling(node->child, child);
    return;
  }

  mdconfremove(child); // Detach child from existing tree

  node->child = child;
  node->child->parent = node;
}

static int mdconfistoken(char c)
{
  return
    c == '_'
    || c == '.'
    || c == '/'
    || c == '\\'
    || c == ':'
    || c == '*'
    || c == '?'
    || c == '$'
    || (c >= 'A' && c <= 'Z')
    || (c >= 'a' && c <= 'z')
    || (c >= '0' && c <= '9')
    ;
}

// Creates a deep copy of a node, with its key, its value and its children
// Node siblings are not copied
mdconfnode *mdconfcopy(const mdconfnode *node)
{
  if(!node) {
    return NULL;
  }

  mdconfnode *copy = (mdconfnode *)malloc(sizeof(mdconfnode));
  copy->key = strdup(node->key);

  if(node->value) {
    copy->value = strdup(node->value);
  } else {
    copy->value = NULL;
  }

  copy->title = node->title;

  copy->prev = NULL;
  copy->next = NULL;
  copy->parent = NULL;

  mdconfnode *dest = copy;
  
  if(node->child) {
    dest->child = mdconfcopy(node->child);
    dest->child->parent = dest;
    dest = dest->child;
    node = node->child;

    while(node->next) {
      dest->next = mdconfcopy(node->next);
      dest->next->prev = dest;
      dest->next->parent = dest->parent;

      dest = dest->next;
      node = node->next;
    }
  }

  return copy;
}

void mdconffree(mdconfnode *root)
{
  // Find real root
  while(root->parent) root = root->parent;
  while(root->prev) root = root->prev;

  // Free recursively
  while(root) {
    if(root->key) {
      free(root->key);
    }

    if(root->value) {
      free(root->value);
    }

    if(root->child) {
      root->child->parent = NULL; // Detach from tree
      mdconffree(root->child);
    }

    mdconfnode *tofree = root;
    root = root->next;
    free(tofree);
  }
}

mdconfnode * mdconfremove(mdconfnode *node) {
  if(node->parent && node->parent->child == node) {
    if(node->prev) node->parent->child = node->prev;
    else if(node->prev) node->parent->child = node->next;
    else {
      node->parent->child = NULL;
    }
  }
  node->parent = NULL;

  if(node->prev) {
    node->prev->next = node->next;
    node->prev = NULL;
  }

  if(node->next) {
    node->next->prev = node->prev;
    node->next = NULL;
  }

  return node;
}

static void mdconfparsekeyvalue(const char *line, mdconfnode *node)
{
  unsigned int keysize = 0;
  const char *key = line;

  while(mdconfistoken(*line)) {
    ++keysize;
    ++line;
  }

  node->key = (char *)malloc(keysize + 1);
  memcpy(node->key, key, keysize);
  node->key[keysize] = '\0';

  // Skip any separator
  while(*line && !mdconfistoken(*line)) {
    ++line;
  }

  if(!*line) {
    node->value = NULL;
    return;
  }

  const char *l;

  if((*line == '*' && line[1] == '*') || (*line == '_' && line[1] == '_')) {
    // Escaped value
    l = line + 2;
    char sep = *line;

    while(*l && l[1] && !(*l == sep && l[1] == sep)) {
      ++l;
    }

    if(!*l) {
      // To end of line : invalid value
      return;
    }

    node->value = (char *)malloc(l - line - 1);
    memcpy(node->value, line + 2, l - line - 2);
    node->value[l - line - 2] = '\0';
    return;
  }

  if(*line == '`') {
    // Escaped value
    l = line + 1;
    char sep = *line;

    while(*l && *l != sep) {
      ++l;
    }

    if(!*l) {
      // To end of line : invalid value
      return;
    }

    node->value = (char *)malloc(l - line);
    memcpy(node->value, line + 1, l - line - 1);
    node->value[l - line - 1] = '\0';
    return;
  }

  for(;;) {
    l = line;

    while(mdconfistoken(*l)) {
      ++l;
    }

    node->value = (char *)malloc(l - line + 1);
    memcpy(node->value, line, l - line);
    node->value[l - line] = '\0';

    // Check for next element in value list
    while(*l && !mdconfistoken(*l)) {
      ++l;
    }

    if(*l) {
      // List of values : create a sibling node with the same key
      const char *key = node->key;
      int title = node->title;
      node = mdconfcreatesibling(node);
      node->key = strdup(key);
      node->title = title;
      line = l;
    } else {
      // Check for include
      while(node && strcasecmp(node->key, "include") == 0) {
        printf("Include [%s] !!!\n", node->value);
        mdconfnode *including = node;
        mdconfnode *included = mdconfparsefile(node->value);
        if(included) {
          mdconfnode *root = node;
          while(root->parent) root = root->parent;

          mdconfnode *n;
          // Append titles to current root
          for(n = included->child; n; n = n->next) {
            mdconfnode *copy = mdconfcopy(n);
            if(n->title) {
              mdconfappendchild(root, copy);
            } else {
              mdconfappendsibling(node, copy);
              node = copy;
            }
          }

          node = including->prev; // process previous include node
          mdconfremove(including);
          mdconffree(including);
          mdconffree(included);
          break;
        }
      }
      break;
    }
  }
}

char *mdconf_prefixes_default[4096] = {
  ".",
  NULL
};
char *(*mdconf_prefixes)[4096] = &mdconf_prefixes_default;

static FILE * openfile(const char *f, const char *mode)
{
  char name[4096];
  strncpy(name, f, 4096);

  FILE *fp = fopen(name, mode);

  if(fp) {
    return fp;
  }

  unsigned int i;

  for(i = 0; (*mdconf_prefixes)[i]; ++i) {
    sprintf(name, "%s/%s", (*mdconf_prefixes)[i], f);
    FILE *fp = fopen(name, mode);

    if(fp) {
      return fp;
    }
  }

  return NULL;
}

mdconfnode *mdconfparsefile(const char *filename)
{
  FILE *f = openfile(filename, "r");

  if(!f) {
    return NULL;
  }

  // Create root node
  mdconfnode *root = (mdconfnode *)malloc(sizeof(mdconfnode));
  root->key = strdup("file");
  root->value = strdup(filename);
  root->next = NULL;
  root->child = NULL;
  root->parent = NULL;

  mdconfnode *curnode = root;
  int curdepth = 0; // 0 == title level, 1 == first list level, 2 == second list level, ...

  int depthonewidth = -1; // Number of spaces for first indentation level

  char lastline[4096] = "";
  int lastlineempty = 1;
  int canbetitle = 0;
  int linebullet = 0;
  int lastlinebullet = 1;
  int lastcanbetitle = 0;

  mdconfnode *listnode = NULL;
  int listnodedepth = -1;

  while(!feof(f)) {
    char line[4096];

    fgets(line, 4096, f);
    char *l = line + strlen(line) - 1;

    while(l >= line) {
      // Strip trailing blanks
      if(*l == '\r' || *l == '\n' || *l == '\t' || *l == ' ') {
        *l = '\0';
      } else if(*l == ')') {
        // Strip comments
        for(l = line; *l != '(' && *l != '\0'; ++l);

        if(*l == '\0') {
          break;
        }

        *l = '\0';
      } else {
        break;
      }

      --l;
    }

    l = line;

    linebullet = 0;

    if(lastlineempty || lastlinebullet) {
      if((l[0] >= 'a' && l[0] <= 'z') || (l[0] >= 'A' && l[0] <= 'Z')) {
        canbetitle = 1;
      } else {
        canbetitle = 0;
      }

      int listdepth = 0;

      // Check for list item
      while(*l == ' ') {
        ++listdepth;
        ++l;
      }

      if((*l == '-' || *l == '*' || *l == '+') && l[1] == ' ') {
        l += 2;

        // Found a bullet
        if(depthonewidth == -1 || listdepth == depthonewidth) {
          depthonewidth = listdepth;

          if(curdepth == 0) {
            curnode = mdconfcreatechild(curnode);
          } else if(curdepth == 1) {
            curnode = mdconfcreatesibling(curnode);
          } else if(curdepth == 2) {
            curnode = mdconfcreatesibling(curnode->parent);
          }
          curnode->title = 0;

          curdepth = 1;
        } else {
          if(curdepth == 0) {
            mdconffree(root);
            return NULL;
          } else if(curdepth == 1) {
            curnode = mdconfcreatechild(curnode);
          } else if(curdepth == 2) {
            curnode = mdconfcreatesibling(curnode);
          }
          curnode->title = 0;

          curdepth = 2;
        }

        mdconfparsekeyvalue(l, curnode);
        linebullet = 1;
      }
    }

    if(lastcanbetitle) {
      canbetitle = 0;

      if(!linebullet && (l[0] == '-' || l[0] == '=')) {
        int i;

        for(i = 1; l[i]; ++i) {
          if(l[i] != l[0]) {
            break;
          }
        }

        if(!l[i]) {
          // Title found
          curnode = mdconfcreatechild(root);
          curnode->title = 1;
          mdconfparsekeyvalue(lastline, curnode);
          curdepth = 0;
          linebullet = 1;
        }
      }
    }

    if(curdepth < listnodedepth) {
      // Fill all children with a copy of listnode child
      mdconfnode *n = listnode;

      if(listnode->child) while(n) {
          n->child = mdconfcopy(listnode->child);
          n = n->next;
        }

      listnode = NULL;
      listnodedepth = -1;
    }

    if(!listnode) {
      if(curnode->next) {
        listnode = curnode;
        listnodedepth = curdepth;
      } else {
        listnodedepth = -1;
      }
    }

    strcpy(lastline, line);
    lastlineempty = 1;
    l = lastline;

    if(*l) {
      lastlineempty = 0;
    }

    lastlinebullet = linebullet;
    lastcanbetitle = canbetitle;
  }

  fclose(f);

  return root;
}

static int tokencmp(const char *token, const char *string, unsigned int stringlen)
{
  size_t tlen = strlen(token);

  if(tlen != stringlen) {
    return 0;
  }

  return strncasecmp(token, string, tlen) == 0;
}

/* Finds first node matching first part of path */
const mdconfnode *mdconfsearch(const mdconfnode *node, const char *path)
{
  if(!node || !path) {
    return NULL;
  }

  const char *namestart = path;

  while(*path && *path != ';' && *path != '=') {
    ++path;
  }

  unsigned int namelen = path - namestart;
  const char *valuestart = NULL;
  int valuelen = -1;

  if(*path == '=') {
    valuestart = ++path;

    while(*path && *path != ';') {
      ++path;
    }

    valuelen = path - valuestart;
  }

  while(node) {
    if(namelen == 0 || tokencmp(node->key, namestart, namelen)) {
      if(!valuestart || (node->value && strncmp(node->value, valuestart, valuelen) == 0)) {
        // Found matching node : descend
        if(*path) {
          const mdconfnode *subnode = mdconfsearch(node->child, path + 1);

          if(subnode) {
            return subnode;
          }
        } else {
          return node;
        }
      }
    }

    node = node->next;
  }

  return NULL;
}

/* Searches depth-first and returns value */
const char *mdconfget(const mdconfnode *root, const char *path)
{
  root = mdconfsearch(root, path);

  if(!root) {
    return NULL;
  }

  return root->value;
}

/* Returns the name of a node by parsing the path or take its title value as a fallback */
const char *mdconfgetvalue(const mdconfnode *node, const char *path)
{
  const char *value = mdconfget(node->child, path);

  if(!value) {
    value = node->value;
  }

  return value;
}

void mdconfprint(const mdconfnode *root, int depth)
{
  if(root->prev) {
    printf("%.*s[%s]: unexpected previous node\n", depth * 2, "                     ", root->key);
  }
  while(root) {
    printf("%.*s[%s]: %s\n", depth * 2, "                     ", root->key, root->value ? root->value : "(null)");

    if(root->child) {
      mdconfprint(root->child, depth + 1);
    }

    root = root->next;
  }
}

/* Test main
int main(int argc, char **argv)
{
  if(argc < 2)
  {
     printf("Usage: %s file.md [pattern]\n", argv[0]);
     return 1;
  }

  mdconfnode *node = mdconfparsefile(argv[1]);
  if(!node)
  {
     printf("Parse error.\n");
     return 2;
  }

  printf("Conf dump :\n");
  mdconfprint(node, 0);

  if(argc < 3)
  {
    return 0;
  }

  const mdconfnode *result = mdconfsearch(node, argv[2]);
  if(result)
  {
    printf("\nPattern search result :\n");
    mdconfprint(result, 0);
  }

  mdconffree(node);
}
//*/

// vim: ts=2 sw=2 sts=2 et

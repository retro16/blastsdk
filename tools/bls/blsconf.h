#ifndef BLSCONF_H
#define BLSCONF_H

void source_parse(group *s, const mdconfnode *n, const char *name);
void output_parse(output *o, const mdconfnode *n, const char *name);
void blsconf_load(const char *file);
void section_parse(section *s, const mdconfnode *md);
void binary_parse(group *b, const mdconfnode *mdnode, const char *name);
char *symname(const char *s);
char *symname2(const char *s, const char *s2);

#endif//BLSCONF_H

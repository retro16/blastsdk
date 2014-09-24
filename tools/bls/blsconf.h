#ifndef BLSCONF_H
#define BLSCONF_H

format format_guess(const char *name);
group *source_parse(const mdconfnode *n, const char *name);
void output_parse(const mdconfnode *n, const char *name);
section *section_parse_ext(const mdconfnode *md, const char *srcname, const char *section_name);
section *section_parse(const mdconfnode *md, const char *srcname, const char *name);
group *binary_parse(const mdconfnode *mdnode, const char *name);
void blsconf_load(const char *file);
char *symname(const char *s);
char *symname2(const char *s, const char *s2);
char *strdupnorm(const char *s, int len);

#endif//BLSCONF_H

// vim: ts=2 sw=2 sts=2 et

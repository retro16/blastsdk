#ifndef BLSCONF_H
#define BLSCONF_H

format format_guess(const char *name);
group * source_parse(const mdconfnode *n, const char *name);
output * output_parse(const mdconfnode *n, const char *name);
section * section_parse_ext(const mdconfnode *md, const char *srcname, const char *section_name);
section * section_parse(const mdconfnode *md, const char *srcname, const char *name);
group * binary_parse(const mdconfnode *mdnode, const char *name);
void blsconf_load(const char *file);
char *symname(const char *s);
char *symname2(const char *s, const char *s2);

#endif//BLSCONF_H

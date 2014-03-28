#include <stdlib.h>
#ifndef BLSLL


#define BLSLL(name) struct blsll_node_##name

#define BLSLL_DECLARE(name, freedata) BLSLL(name) { name *data; BLSLL(name) *next; }; \
  static inline BLSLL(name) * blsll_insert_##name(BLSLL(name) *list, name *data) { \
    BLSLL(name) * node = (BLSLL(name) *)malloc(sizeof(BLSLL(name))); \
    node->data = data; node->next = list; return node; } \
  static inline BLSLL(name) * blsll_create_##name(BLSLL(name) *list) { \
    BLSLL(name) * node = (BLSLL(name) *)malloc(sizeof(BLSLL(name))); \
    node->data = (name *)calloc(1, sizeof(name)); node->next = list; return node; } \
  static inline void blsll_free_##name(BLSLL(name) *list) { BLSLL(name) *next; \
    for(; (list); (list) = next) { next = (list)->next; free(list); } } \
  static inline void blsll_freedata_##name(BLSLL(name) *list) { BLSLL(name) *next; \
    for(; (list); (list) = next) { next = (list)->next; if((list)->data) freedata((list)->data); free(list); } } \
  static inline BLSLL(name) * blsll_copy_##name(BLSLL(name) *list, BLSLL(name) *append) { BLSLL(name) *prevnode = NULL, *node; \
    for(; (list); (list) = (list)->next) { node = (BLSLL(name) *)malloc(sizeof(BLSLL(name))); node->data = list->data; node->next = append; if(prevnode) prevnode->next = node; prevnode = node; } return node; } \

#define BLSLL_FOREACH(var, list) for(; (list) && ((var = list->data), list); (list) = (list)->next)
#define BLSLL_FIND(list, field, value) for(; (list) && (!(list)-data || ((list)->data->(field) != (value))), (list) = (list)->next);
#define BLSLL_FINDSTR(list, field, value) for(; (list) && (!(list)->data || (strcmp((list)->data->(field), (value)))), (list) = (list)->next);
#define BLSLL_FINDSTRCASE(list, field, value) for(; (list) && (!(list)->data || (strcasecmp((list)->data->(field), (value)))), (list) = (list)->next);


#define BLSENUM(name, size) extern const char name##_names[][size];\
  static inline name name##_parse(const char *s) { if(!s) return (name)0; \
  int i; for(i = 0; i < name##_max; ++i) \
    if(strcmp(s, name##_names[i]) == 0) return (name)i; \
  return (name)0; }

#endif

#ifndef BLSLL

#define BLSLL(name) \
 struct name * name##_create() { return (struct name *)calloc(1, sizeof(struct name)); } \
 static inline struct name * name##_append(struct name * node) { \
   struct node * next = name##_create(); \
   next->next = node->next; \
   node->next = next; \
   return next; } \
 void name##_free(struct name *) \
 struct name * name##_copy(struct name *) \
 static inline struct name * name##_copylist(struct name *l) { \
   struct name *first = name##_copy(l); \
   struct name *tgt = first;
   while(l->next) { tgt->next = name##_copy(l->next); tgt = tgt->next; l = l->next; } }

#define BLSLL_FOREACH(var, list) for((var) = (list); (var); (var) = (var)->next)
#define BLSLL_FIND(var, field, value) for(; var && (var)->field != (value); (var) = (var)->next)
#define BLSLL_FINDSTR(var, field, value) for(; var && strcmp((var)->field, (value)); (var) = (var)->next)
#define BLSLL_FINDSTRCASE(var, field, value) for(; var && strcasecmp((var)->field, (value)); (var) = (var)->next)


#define BLSENUM(name) static inline name##_t name##_parse(const char *s) { if(!s) return (name##_t)0; \
  int i; \
  for(i = 0; i < sizeof(name##_names)/sizeof(name##_names[0]); ++i) \
    if(strcmp(s, name##_names[i]) == 0) return (name##_t)i; \
  return (name##_t)0; }




typedef struct blsll_node {
  void *value;
  struct blsll_node *next;
} blsll_node_t;


#endif

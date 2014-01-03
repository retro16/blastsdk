#ifndef BLSLL

#define BLSLL(name) \
 struct name * name##_create(); \
 static inline struct name * name##_append(struct name * node) { \
   struct node * next = name##_create(); \
   next->next = node->next; \
   node->next = next; \
   return next; } \
 void name##_free(struct name *)

#define BLSLL_FOREACH(var, list) for((var) = (list); (var); (var) = (var)->next)
#define BLSLL_FIND(var, field, value) for(; var && (var)->field != (value); (var) = (var)->next)
#define BLSLL_FINDSTR(var, field, value) for(; var && strcmp((var)->field, (value)); (var) = (var)->next)
#define BLSLL_FINDSTRCASE(var, field, value) for(; var && strcasecmp((var)->field, (value)); (var) = (var)->next)


#endif

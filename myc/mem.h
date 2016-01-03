#ifndef MEM_H
#define MEM_H

void free_var(VAR*);
void free_list(LIST*);
void free_elts(LIST*);
LIST* new_elt(void);
VAR* new_var(void);
VEC* new_vec(int);
FN* new_fn(void);
char* strsave(char*);
void print_mem(void);
void env_add(HASH*);
HASH* new_hash(int);
void gc(void);
void* mal_malloc(size_t);
LIST* ref_elt(LIST*);
void add_active(VAR*);
void del_active(int);

#endif

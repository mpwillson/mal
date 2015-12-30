#ifndef HASH_H
#define HASH_H

struct s_sym {
	char *name;
	struct s_var *value;
	struct s_sym *next;
};

struct s_hash {
    int size;
    bool closure;
    struct s_hash *outer;
    struct s_sym **sym;
};
    
typedef struct s_hash HASH;
typedef struct s_sym SYM;

struct s_iter {
    HASH* hash;
    int index;
    SYM* sp;
};

typedef struct s_iter ITER;

HASH* new_env(int,HASH*,VAR*,VAR*);
/* static int hashed(HASH*,char*); */
/* static SYM* lookup(HASH*,char*); */
HASH* env_put(HASH*,char*,VAR*);
VAR* env_get(HASH*,char*);
void env_dump(HASH*);
void env_free(HASH*);
ITER* env_iter_init(HASH*);
SYM* env_next(ITER*);
HASH* env_del(HASH*,char*);

#endif

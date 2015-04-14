#ifndef ENV_H
#define ENV_H

struct s_sym {
	char *name;
	struct s_var *value;
	struct s_sym *next;
};

struct s_env {
    int size;
    bool closure;
    struct s_env *outer;
    struct s_sym **sym;
};
    
typedef struct s_env ENV;
typedef struct s_sym SYM;

ENV* new_env(int,ENV*,VAR*,VAR*);
int hash(ENV*,char*);
SYM* lookup(ENV*,char*);
ENV* env_put(ENV*,char*,VAR*);
VAR* env_get(ENV*,char*);
void env_dump(ENV*);
void env_free(ENV*);

#endif

#ifndef ENV_H
#define ENV_H

struct s_sym {
	char *name;
	struct s_var *value;
	struct s_sym *next;
};

struct s_env {
    int size;
    struct s_sym *sym;
};

    
typedef struct s_env ENV;
typedef struct s_sym SYM;

#endif

#ifndef MAL_H
#define MAL_H

/* token and var types */
#define S_UNDEF -1
#define S_INT 0
#define S_EOE 1
#define S_EOF 2
#define S_STR 3
#define S_VAR 4
#define S_USTR 5 /* unterminated string */
#define S_FLOAT 6
#define S_LIST 7
#define S_ROOT 8 /* a list that is not a list */
#define S_KEYWORD 9
#define S_QUOTE 10
#define S_QUASIQUOTE 11
#define S_UNQUOTE 12
#define S_SPLICE 13
#define S_ARRAY 14
#define S_HASHMAP 15
#define S_META 16
#define S_DEREF 17

/* Forward references */
struct s_list;
struct s_var;
typedef struct s_var VAR;
typedef struct s_list LIST;
/* declaration for functions that do stuff to two VARs */
typedef VAR*(*FUN)(VAR*,VAR*);

union u_val {
	char *pval;
	int ival;
	float fval;
    struct s_list *lval;
};

struct s_var {
	int type;
    FUN function;
	union u_val val;
};

struct s_list {
    VAR *var;
    struct s_list *next;
};

/* function prototypes */
void mal_error(char*);

#endif
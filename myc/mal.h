#ifndef MAL_H
#define MAL_H

/* token and var types */
#define S_UNDEF -1
#define S_INT 0
#define S_EOE 1
#define S_EOF 2
#define S_STR 3
#define S_SYM 4
#define S_USTR 5 /* unterminated string */
#define S_REAL 6
#define S_LIST 7
#define S_ROOT 8 /* a list that is not a list */
#define S_KEYWORD 9
#define S_QUOTE 10
#define S_QUASIQUOTE 11
#define S_UNQUOTE 12
#define S_SPLICE 13
#define S_VECTOR 14
#define S_HASHMAP 15
#define S_META 16
#define S_DEREF 17
#define S_ERROR 18
#define S_NIL 19
#define S_TRUE 20
#define S_FALSE 21
#define S_FN 22

#define islist(t) (t == S_ROOT || t == S_LIST || t == S_VECTOR || \
                      t == S_HASHMAP)
#define isstr(t) (t == S_STR || t == S_SYM || t == S_KEYWORD)

/* Forward references */
struct s_list;
struct s_var;
struct s_fn;
struct s_env;

typedef struct s_var VAR;
typedef struct s_list LIST;
typedef struct s_fn FN;

/* declaration for internal functions that do stuff to two VARs */
typedef VAR*(*FUN)(VAR*,VAR*);

union u_val {
	char *pval;
	int ival;
	double rval;
    FN* fval;
    LIST* lval;
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

struct s_fn {
    VAR* args;
    LIST* forms;
    struct s_env *env;
};


/* Pre-defined VARs */
static VAR quote = {
    S_SYM,NULL,"quote"
};
static VAR quasiquote = {
    S_SYM,NULL,"quasiquote"
};
static VAR unquote = {
    S_SYM,NULL,"unquote"
};
static VAR splice = {
    S_SYM,NULL,"splice-unquote"
};
static VAR deref = {
    S_SYM,NULL,"deref"
};
static VAR meta = {
    S_SYM,NULL,"with-meta"
};
static VAR var_nil = {
    S_NIL,NULL,NULL
};
static VAR var_true = {
    S_TRUE,NULL,NULL
};
static VAR var_false = {
    S_FALSE,NULL,NULL
};
static VAR var_do = {
    S_SYM,NULL,"do"
};

/* function prototypes */
char* mal_error(const char *fmt, ...);
void mal_die(char*);
void free_var(VAR*);
void free_list(LIST*);

#endif

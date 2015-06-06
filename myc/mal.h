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
#define S_BUILTIN 23
#define S_COMMENT 24
#define S_MACRO 25

#define islist(t) (t == S_ROOT || t == S_LIST || t == S_VECTOR)/* ||    \
                        t == S_HASHMAP) */
#define isstr(t) (t == S_STR || t == S_SYM || t == S_KEYWORD)

/* Forward references */
struct s_list;
struct s_var;
struct s_fn;
struct s_hash;

typedef struct s_var VAR;
typedef struct s_list LIST;
typedef struct s_fn FN;
typedef struct s_vec VEC;

/* declaration for internal functions that do stuff to LIST */
typedef VAR*(*BUILTIN)(LIST*);

union u_val {
	char *pval;
	int ival;
	double rval;
    FN* fval;
    LIST* lval;
    BUILTIN bval;
    VEC* vval;
    struct s_hash* hval;
    
};

struct s_var {
	int type;
	union u_val val;
    bool marked;
};

struct s_list {
    VAR *var;
    struct s_list *next;
};

struct s_fn {
    VAR* args;
    VAR* forms;
    struct s_hash *env;
};

struct s_vec {
    int size;
    VAR** vector;
};

    
/* Declarations for pre-defined atoms */
extern VAR quote;
extern VAR quasiquote;
extern VAR unquote;
extern VAR splice;
extern VAR deref;
extern VAR meta;
extern VAR var_nil;
extern VAR var_true;
extern VAR var_false;
extern VAR error;
extern VAR empty_list;

/* function prototypes */
VAR* mal_error(const char *fmt, ...);
void mal_die(char*);
LIST* append(LIST*,VAR*);
VAR* list2var(LIST*);
VAR* repl_read(char *);
VAR* eval(VAR*,struct s_hash *);
VAR* eval_ast(VAR*,struct s_hash *);
VAR* first(VAR*);
VAR* second(VAR*);
VAR* rest(VAR*);
VAR* but_last(LIST*);
VAR* last(LIST*);
void throw(VAR*);
VEC* mkvector(LIST*);
VAR* seq(VAR*);
struct s_hash* mkhashmap(LIST*);
VAR* str2var(char*);

#endif

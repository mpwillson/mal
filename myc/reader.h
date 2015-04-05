/* reader.h - mal reader header file */

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

#define LEXTOKSIZ 72
#define LEXBUFSIZ 255


struct s_list;

union u_val {
	char *pval;
	int ival;
	float fval;
    struct s_list *lval;
};

struct s_var {
	int type;
	union u_val val;
};

typedef struct s_var VAR;

struct s_list {
    VAR *var;
    struct s_list *next;
};

typedef struct s_list LIST;


extern char lextok[];

extern int lexer(void);
extern void init_lexer(char *s);
extern char* list_open(int);
extern char* list_close(int);

/* function prototypes */

VAR* read_atom(int,char*);
LIST* new_elt(void);
VAR* new_var(void);
LIST* append(LIST*,VAR*);
VAR* read_list(int,char);
VAR* read_form(int);
VAR* insert(VAR*,VAR*);

#define TRUE 1
#define FALSE 0

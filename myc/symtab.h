/* symtab.h */

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

/* function prototypes */

int defvar(char *);
int setvar(char *,struct s_var *);
int getvar(char *,struct s_var *);
void dmpvar();
void initvar(void);
VAR* symbolise(int,char*);
LIST* new_list(void);
VAR* new_var(void);
LIST* append(LIST*,VAR*);
VAR* read_list(int,char,char);
VAR* insert(VAR*,VAR*);

#define TRUE 1
#define FALSE 0

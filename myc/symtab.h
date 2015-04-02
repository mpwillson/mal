/* symtab.h */

struct s_list;

union u_val {
	int ival;
	float fval;
	char *pval;
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

#define TRUE 1
#define FALSE 0

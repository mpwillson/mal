/* reader.h - mal reader header file */

#define LEXTOKSIZ 72

extern char lextok[];

/* function prototypes */
extern int lexer(void);
extern void init_lexer(char *s);
extern char* list_open(int);
extern char* list_close(int);
VAR* read_atom(int,char*);
VAR* read_list(int,char);
VAR* read_form(int);

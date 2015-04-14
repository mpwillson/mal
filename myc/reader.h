/* reader.h - mal reader header file */

#define LEXTOKSIZ 72
#define LEXBUFSIZ 255

extern char lextok[];

/* function prototypes */
extern int lexer(void);
extern void init_lexer(char *s);
extern char* list_open(int);
extern char* list_close(int);
char* strsave(char*);
VAR* read_atom(int,char*);
LIST* new_elt(void);
VAR* new_var(void);
LIST* append(LIST*,VAR*);
VAR* insert(VAR*,VAR*);
VAR* read_list(int,char);
VAR* read_form(int);
VAR* insert(VAR*,VAR*);

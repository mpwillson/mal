/* lexer.h - lexical analysis header file */

#define S_UNDEF -1
#define S_INT 0
#define S_EOE 1
#define S_EOF 2
#define S_STR 3
#define S_VAR 4
#define S_USTR 5 /* unterminated string */
#define S_GTEQ 6
#define S_LTEQ 7
#define S_NEQ 8
#define S_EQ 9

#define LEXTOKSIZ 72
#define LEXBUFSIZ 255

extern int lexsym;
extern char lextok[];

extern void lexer(void);
extern void lexmatch(char ch);
extern void init_lexer(char *s);



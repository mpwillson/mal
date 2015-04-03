/* reader.h - lexical analysis header file */

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

#define LEXTOKSIZ 72
#define LEXBUFSIZ 255

extern char lextok[];

extern int lexer(void);
extern void init_lexer(char *s);

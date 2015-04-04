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
#define S_QUASIQUOTE 11
#define S_UNQUOTE 12
#define S_SPLICE 13
#define S_ARRAY 14
#define S_HASHMAP 15
#define S_META 16
#define S_DEREF 17

#define LEXTOKSIZ 72
#define LEXBUFSIZ 255

extern char lextok[];

extern int lexer(void);
extern void init_lexer(char *s);
extern char* list_open(int);
extern char* list_close(int);

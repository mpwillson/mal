/*
 * 	NAME
 *      reader - lexical scanning for mal
 * 	
 * 	SYNOPSIS
 *      void init_lexer(char *buf);
 *          Initialise lexer input buffer.
 *          
 *      void lexer();
 *      	determine next token.  Global variable lexsym holds token
 *      	type, lextok holds token content
 *
 * 	    void lexmatch(char sym);
 * 	        moves to next symbol, after checking current symbol is sym.
 * 	        
 * 	DESCRIPTION
 *
 *
 * 	NOTES
 * 		Single character operators stand for themselves as
 * 		tokens. Other tokens are assigned special symbols, beginning
 * 		with S_. 
 *      TBD - place all lexical scanning elements into separate functions
 *
 * 	MODIFICATION HISTORY
 * 	Mnemonic		Rel	Date	Who
 *	lexer           1.0 020924  mpw
 *		Created.
 *
 */

#include <stdio.h>
#include "reader.h"

/* GLOBALS */

int lexsym;
char lextok[LEXTOKSIZ];

/* END GLOBALS */

static char lexbuf[LEXBUFSIZ];
static int bufidx;

#define getlexchar() ((lexbuf[bufidx] != '\0')?lexbuf[bufidx++]:EOF)
/* never push back EOF */
#define ungetlexchar() (bufidx--)

void init_lexer(char *s)
{
	strcpy(lexbuf,s);
    
    /* put don't care character at end of expression, otherwise
     * lexchar macros behave badly (sometimes you need to push back
     * EOF and sometimes you don't).
     */
    strcat(lexbuf,"\n");        
	bufidx = 0;
}


void lexer(void)
{
	char ch = ' ';
	int i = 0;

	lextok[0] = '\0';
	while (ch == ' ' || ch == '\t') ch = getlexchar();
	if (isdigit(ch)) {
		while (isdigit(ch) && i < LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		ungetlexchar();
		lextok[i] = '\0';
		lexsym = S_INT;
	}
	else if (isalpha(ch)) {
		while ((isalnum(ch) || ch == '_') && i <LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		ungetlexchar();
		lextok[i] = '\0';
		lexsym = S_VAR;
	}
	else if (ch == '"') {
		ch = getlexchar();
		while (ch != '\n' && ch != EOF && ch != '"' && i < LEXTOKSIZ) {
			if (ch == '\\') ch = getlexchar();
			lextok[i++] = ch;
			ch = getlexchar();
		}
		if (ch != '"') {
			ungetlexchar();
			lexsym = S_USTR; 	/* unterminated string */
		}
		else {
			lexsym = S_STR;
		}
		lextok[i] = '\0';
	}
	else if (ch == '(' || ch == ')') {
		lexsym = ch;
		lextok[0] = ch; lextok[1] = '\0';
	}
    else if (ch == EOF) {
		lexsym = S_EOF;
		strcpy(lextok,"end-of-file");
	}
	else if (ch == '\n') {
		lexsym = S_EOE;
		strcpy(lextok,"newline");
	}
	else {
		/* unrecognised token */
		while (ch != '\n' && ch != EOF) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		lextok[i] = '\0';
		ungetlexchar();
		lexsym = S_UNDEF;
	}
/*  	fprintf(stderr,"lexsym: %d, lextok: %s\n",lexsym,lextok); */
    
}

void lexmatch(char ch)
{
	if (ch == lexsym) {
		lexer();
	}
	else {
		fprintf(stderr,"expected '%c'\n",ch);
	}
}

		
	

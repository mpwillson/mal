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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "reader.h"

/* GLOBALS */

char lextok[LEXTOKSIZ+1];

/* END GLOBALS */

static char lexbuf[LEXBUFSIZ+1];
static int bufidx;

#define getlexchar() ((lexbuf[bufidx] != '\0')?lexbuf[bufidx++]:EOF)
#define ungetlexchar() (bufidx--)

void init_lexer(char *s)
{
    bufidx = 0;
    strncpy(lexbuf,s,LEXBUFSIZ);
}

int issymbol(char ch)
{
    return (isalnum(ch) || (strchr("/!$%&*_-+=<>?",ch) != NULL));
}

int lexer(void)
{
	char ch = ' ';
	int i = 0, lexsym;

	lextok[0] = '\0';
	while (ch == ' ' || ch == ',' || ch == '\t') ch = getlexchar();
	if (isdigit(ch)) {
		lexsym = S_INT;
		while ((isdigit(ch) || ch == '.') && i < LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		ungetlexchar();
		lextok[i] = '\0';
        if (strchr(lextok,'.') != NULL) lexsym = S_FLOAT;
	}
	else if (issymbol(ch) || ch == ':') {
        if (ch == ':') {
            lexsym = S_KEYWORD;
            ch = getlexchar();
        }
        else {
            lexsym = S_VAR;
        }
        while (issymbol(ch) && i<LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		ungetlexchar();
		lextok[i] = '\0';
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
	else if (ch == '(' || ch == ')' || ch == '[' || ch == ']' ||
             ch == '{' || ch == '}') {
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
    else if (ch == '\'') {
        lexsym = S_QUOTE;
        strcpy(lextok,"quote");
    }
    else if (ch == '`') {
        lexsym = S_QUASIQUOTE;
        strcpy(lextok,"quasiquote");
    }
    else if (ch == '~') {
        ch = getlexchar();
        if (ch == '@') {
            lexsym = S_SPLICE;
            strcpy(lextok,"splice-unquote");
        }
        else {
            ungetlexchar();
            lexsym = S_UNQUOTE;
            strcpy(lextok,"unquote");
        }
    }
    else if (ch == ';') {
        lextok[0] = '\0';
        lexsym = S_EOF;
    }
    else if (ch == '@') {
        lexsym = S_DEREF;
        strcpy(lextok,"deref");
    }
    else {
		/* unrecognised token */
		while (ch != '\n' && ch != EOF && i < LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		lextok[i] = '\0';
		ungetlexchar();
		lexsym = S_UNDEF;
	}
    return lexsym;
}

char* list_open(int type)
{
    switch (type) {
        case S_LIST:
            return "(";
        case S_ARRAY:
            return "[";
        case S_HASHMAP:
            return "{";
    }
}
char* list_close(int type)
{
    switch (type) {
        case S_LIST:
            return ")";
        case S_ARRAY:
            return "]";
        case S_HASHMAP:
            return "}";
    }
}

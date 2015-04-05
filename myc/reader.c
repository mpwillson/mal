/*
 * 	NAME
 *      reader - lexical scanning and reader for mal
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
 *
 *      
 * 	MODIFICATION HISTORY
 * 	Mnemonic		Rel	Date	Who
 *	reader          1.0 020924  mpw
 *		Created.
 *
 */

#include <ctype.h>
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
    else if (ch == '^') {
        lexsym = S_META;
        strcpy(lextok,"with-meta");
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
    return "";
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
    return "";
    
}

char *strsave(char *s)
{
	char *p;

	p = (char *) malloc(strlen(s)+1);
	if (p != NULL) 	strcpy(p,s);
	return p;
}


/* Return atom, created from type and value */
VAR* read_atom(int type,char *s)
//VAR *symbolise(int type, char *s)
{
    VAR *new = new_var();

    new->type = type;
    switch (type) {
        case S_INT: 
            new->val.ival = strtol(s,NULL,10);
            break;
        case S_FLOAT:
            new->val.fval = strtod(s,NULL);
            break;
        case S_VAR:
        case S_STR:
        case S_KEYWORD:
            new->val.pval = strsave(s);
            break;
    }   
    return new;
}

LIST* new_elt() {
    LIST* elt;
    
    elt = (LIST *) malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_error("out of memory at new_elt.");
    }
    elt->var = NULL;
    elt->next = NULL;
    return elt;
}

VAR* new_var() {
    VAR* var;
    
    var = (VAR *) malloc(sizeof(VAR));
    if (var == NULL)  {
        mal_error("out of memory at new_var.");
    }
    var->type = S_UNDEF;
    var->val.lval = NULL;
    return var;
}

VAR* insert(VAR* var, VAR* list)
{
    LIST* elt = new_elt();

    elt->var = var;
    elt->next = list->val.lval;
    list->val.lval = elt;
    return list;
}


LIST* append(LIST* list,VAR* var)
{
    LIST *elt,*current;

    elt = new_elt();
    elt->var = var;
    if (list == NULL) {
        return elt;
    }
    else {
        current = list;
        while (current->next != NULL) current = current->next;
        current->next = elt;
    }
    return list;
}

static VAR quote = {
    S_VAR,
    "quote"
};
static VAR quasiquote = {
    S_VAR,
    "quasiquote"
};
static VAR unquote = {
    S_VAR,
    "unquote"
};
static VAR splice = {
    S_VAR,
    "splice-unquote"
};
static VAR deref = {
    S_VAR,
    "deref"
};
static VAR meta = {
    S_VAR,
    "with-meta"
};

VAR* handle_quote(int token_type)
{
    VAR* quote_type;

    switch (token_type) {
        case S_QUOTE:
            quote_type = &quote;
            break;
        case S_QUASIQUOTE:
            quote_type = &quasiquote;
            break;
        case S_UNQUOTE:
            quote_type = &unquote;
            break;
        case S_SPLICE:
            quote_type = &splice;
            break;
        case S_DEREF:
            quote_type = &deref;
            break;
    }
    return insert(quote_type,read_list(S_LIST,')'));
}

VAR* handle_meta()
{
    VAR* meta_form = read_form(lexer());
    VAR* object_form = read_form(lexer());
    LIST* new = NULL;
    VAR* var = new_var();

    new = append(new,&meta);
    new = append(new,object_form);
    new = append(new,meta_form);
    var->type = S_LIST;
    var->val.lval = new;
    return var;
}

VAR* read_list(int type,char close)
{
    LIST* list = NULL;
    VAR* var = new_var();
    int token_type;

    token_type = lexer();
    while (token_type != S_EOE && token_type != S_EOF && token_type != close) {
        list = append(list,read_form(token_type));
        token_type = lexer();
    }
    var->type = type;
    var->val.lval = list;
    return var;
}

VAR* read_form(int token_type)
{
    LIST* form = NULL;
    VAR* var;

    var = new_var();
    switch (token_type) {
        case '(':
            var = read_list(S_LIST,')');
            break;
        case '[':
            var = read_list(S_ARRAY,']');
            break;
        case '{':
            var = read_list(S_HASHMAP,'}');
            break;
        case S_QUOTE:
        case S_QUASIQUOTE:
        case S_UNQUOTE:
        case S_SPLICE:
        case S_DEREF:
            var = handle_quote(token_type);
            break;
        case S_META:
            var = handle_meta();
            break;
        default:
            var = read_atom(token_type,lextok);
    }       
    return var;
}

/*
 * 	NAME
 *      reader - lexical scanning and reader for mal
 * 	
 * 	SYNOPSIS
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
 *	reader          1.0 150405  mpw
 *		Created.
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mal.h"
#include "reader.h"

/* GLOBALS */

char lextok[LEXTOKSIZ+1];

/* END GLOBALS */

static char lexbuf[LEXBUFSIZ+1];
static int bufidx;

#define getlexchar() ((lexbuf[bufidx] != '\0')?lexbuf[bufidx++]:EOF)
#define ungetlexchar(ch) ((ch != '\0' && ch != EOF)?bufidx--:0)


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
		ungetlexchar(ch);
		lextok[i] = '\0';
        if (strchr(lextok,'.') != NULL) lexsym = S_REAL;
	}
	else if (issymbol(ch) || ch == ':') {
        if (ch == ':') {
            lexsym = S_KEYWORD;
            ch = getlexchar();
        }
        else {
            lexsym = S_SYM;
        }
        while (issymbol(ch) && i<LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		ungetlexchar(ch);
		lextok[i] = '\0';
        if (strcmp(lextok,"nil") == 0) {
            lexsym = S_NIL;
        }
        else if (strcmp(lextok,"true") == 0) {
            lexsym = S_TRUE;
        }
        else if(strcmp(lextok,"false") == 0) {
            lexsym = S_FALSE;
        }
	}
	else if (ch == '"') {
		ch = getlexchar();
		while (ch != '\n' && ch != EOF && ch != '"' && i < LEXTOKSIZ) {
			if (ch == '\\') {
                ch = getlexchar();
                if (ch == 'n') {
                    ch = '\n';
                }
            }
            lextok[i++] = ch;
            ch = getlexchar();
		}
		if (ch != '"') {
			ungetlexchar(ch);
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
	else if (ch == '\n' || ch == '\0') {
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
            ungetlexchar(ch);
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
		while (ch != '\n' && ch != EOF && ch != '\0' && i < LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		lextok[i] = '\0';
		ungetlexchar(ch);
		lexsym = S_UNDEF;
	}
    return lexsym;
}

char* list_open(int type)
{
    switch (type) {
        case S_LIST:
            return "(";
        case S_VECTOR:
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
        case S_VECTOR:
            return "]";
        case S_HASHMAP:
            return "}";
    }
    return "";
    
}

/* Return atom, created from type and value */
VAR* read_atom(int type,char *s)
{
    VAR *new;;

    new->type = type;
    switch (type) {
        case S_INT:
            new = new_var();
            new->type = type;
            new->val.ival = strtol(s,NULL,10);
            break;
        case S_REAL:
            new = new_var();
            new->type = type;
            new->val.rval = strtod(s,NULL);
            break;
        case S_SYM:
        case S_STR:
        case S_KEYWORD:
            new = new_var();
            new->type = type;
            new->val.pval = strsave(s);
            break;
        case S_NIL:
            return &var_nil;
        case S_TRUE:
            return &var_true;
        case S_FALSE:
            return &var_false;
    }   
    return new;
}

VAR* handle_quote(int token_type)
{
    VAR* quote_type;
    VAR* list;
    VAR* new = new_var();
    LIST* elt;
    
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
    list = read_form(lexer());
    if (list->type == S_ERROR) {
        return list;
    }
    else if (list->type != S_LIST) {
        /* turn into list, so quote atom can be inserted */
        elt = append(NULL,list);
        new->type = S_LIST;
        new->val.lval = elt;
        return insert(quote_type,new);
    }
    else {
        return insert(quote_type,list);
    }
}

VAR* handle_meta()
{
    VAR* meta_form = read_form(lexer());
    VAR* object_form = read_form(lexer());
    LIST* new = NULL;
    VAR* var = new_var();

    if (meta_form->type == S_ERROR) return meta_form;
    if (object_form->type == S_ERROR) return object_form;
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
    if (type != S_ROOT && token_type != close) {
        var->type = S_ERROR;
        var->val.pval = mal_error("terminating list character '%c' expected",
                                  close);
    }
    else {
        var->type = type;
        var->val.lval = list;
    }
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
            var = read_list(S_VECTOR,']');
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
        case S_USTR:
            var->type = S_ERROR;
            var->val.pval = mal_error("unterminated string");
            break;
        default:
            var = read_atom(token_type,lextok);
    }       
    return var;
}

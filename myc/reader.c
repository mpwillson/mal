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
#include <stdbool.h>

#include "mal.h"
#include "reader.h"
#include "env.h"
#include "mem.h"

/* GLOBALS */

char lextok[LEXTOKSIZ+1];

/* END GLOBALS */

char* lexbuf = NULL;
static int bufidx = 0;
static int lexbufsize = 0;

#define getlexchar() ((lexbuf[bufidx] != '\0')?lexbuf[bufidx++]:EOF)
#define ungetlexchar(ch) ((ch != '\0' && ch != EOF)?bufidx--:0)


void init_lexer(char *s)
{
    int slen;

    slen = strlen(s);
    if (slen >= lexbufsize) {
        if (lexbuf) {
            lexbuf = (char *) realloc(lexbuf,slen+1);
        }
        else {
            lexbuf = (char *) malloc(slen+1);
        }
    }
    lexbufsize = slen+1;
    bufidx = 0;
    strncpy(lexbuf,s,slen+1);
}

int issymbol(char ch)
{
    return (isalnum(ch) || (strchr("/!$%&*_-+=<>?",ch) != NULL));
}

int lexer(void)
{
	char ch = ' ';
	int i = 0, lexsym;
    bool minus;
    
    minus = false;
	lextok[0] = '\0';
	while (ch == ' ' || ch == ',' || ch == '\t' || ch == '\n') {
        ch = getlexchar();
    }

    /* check for negative number */
    if (ch == '-') {
        ch = getlexchar();
        if (isdigit(ch) || ch == '.') {
            minus = true;
        }
        else {
            ungetlexchar(ch);
            ch = '-';
        }
    }
	if (isdigit(ch)) {
		lexsym = S_INT;
        if (minus) lextok[i++] = '-';
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
		while (ch != '\n' && ch != EOF && ch != '\0' && i < LEXTOKSIZ) {
			lextok[i++] = ch;
			ch = getlexchar();
		}
		lextok[i] = '\0';
		ungetlexchar(ch);
		lexsym = S_COMMENT;
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
    VAR *new;

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
        case S_NIL:
            return &var_nil;
        case S_TRUE:
            return &var_true;
        case S_FALSE:
            return &var_false;
        case S_SYM:
            /* handle (some) special forms */   
            if (strcmp(s,"quote")==0) {
                return &quote;
            }
            else if (strcmp(s,"quasiquote")==0) {
                return &quasiquote;
            }
            else if (strcmp(s,"unquote")==0) {
                return &unquote;
            }
            else if (strcmp(s,"splice-unquote")==0) {                
                return &splice;
            }
            else if (strcmp(s,"deref")==0) {                
                return &deref;
            }
            else if (strcmp(s,"do") == 0) {
                return &do_sym;
            }
            else if (strcmp(s,"def!") == 0) {
                return &def;
            }
            else if (strcmp(s,"let*") == 0) {
                return &let;
            }
            else if (strcmp(s,"if") == 0) {
                return &if_sym;
            }
            else if (strcmp(s,"fn*") == 0) {
                return &fn_sym;
            }
            else if (strcmp(s,"defmacro!") == 0) {
                return &defmacro;
            }
            else if (strcmp(s,"macroexpand") == 0) {
                return &macroexpand_sym;
            }
            else if (strcmp(s,"try*") == 0) {
                return &try;
            }
            else if (strcmp(s,"catch*") == 0) {
                return &catch;
            }
            /* fall through */
        default:
            new = new_var();
            new->type = type;
            new->val.pval = strsave(s);
    }   
    return new;
}

VAR* handle_quote(int token_type)
{
    VAR* quote_type;
    VAR* form;
    
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
    form = read_form(lexer());
    return list2var(append(append(NULL,quote_type),form));
}

VAR* handle_meta()
{
    VAR* meta_form = read_form(lexer());
    VAR* object_form = read_form(lexer());
    LIST* new = NULL;

    new = append(new,&meta);
    new = append(new,object_form);
    new = append(new,meta_form);
    return list2var(new);
}

VAR* read_list(int type,char close)
{
    LIST* list = NULL;
    VAR* var;
    int token_type;

    token_type = lexer();
    while (token_type != S_EOE && token_type != S_EOF && token_type != close) {
        var = read_form(token_type);
        if (var->type != S_COMMENT) list = append(list,var);
        token_type = lexer();
    }
    if (type != S_ROOT && token_type != close) {
        throw(mal_error("terminating list character '%c' expected",close));
    }
    var = new_var();
    if (type == S_VECTOR) {
        var->val.vval = mkvector(list);
        free_elts(list);
    }
    else if (type == S_HASHMAP) {
        var->val.hval = mkhashmap(list);
        free_elts(list);
        if (!var->val.hval) throw(mal_error("odd number of forms for hashmap"));
    }
    else {
        var->val.lval = ref_elt(list);
    }
    var->type = type;
    return var;
}

VAR* read_form(int token_type)
{
    VAR* var = NULL;

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
            throw(mal_error("unterminated string"));
            break;
        case S_COMMENT:
            var = new_var();
            var->type = S_COMMENT;
            var->val.pval = strsave(lextok);
            break;
        case S_EOF:
            var = &var_nil;
            break;
        default:
            var = read_atom(token_type,lextok);
    }       
    return var;
}

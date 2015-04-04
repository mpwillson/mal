/* symtab.c: handles storing and retrieving of variable names 
             
   Contains the following routines:
              
   defvar  - defines a variable name (puts in symbol table)
   setvar  - sets a value for a defined variable name
   getvar  - returns a value for a defined variable name
   dmpvar -  dumps contents of symbol table
                           
   Variable types are int or character.
             
   created 27/09/90 MPW

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "reader.h"
#include "symtab.h"

#define HASHSIZE 101

/* define symbol table */
static struct s_sym {
	char *name;
	struct s_var *value;
	struct s_sym *next;
} *symtab[HASHSIZE];

int debug;

char *strsave(char *s)
{
	char *p;

	p = (char *) malloc(strlen(s)+1);
	if (p != NULL) 	strcpy(p,s);
	return p;
}

int hash(char *s)       /* return hash value for string */
{
	int hashval;
   
	for (hashval=0; *s != '\0'; ) hashval += *s++;
	return (hashval%HASHSIZE);
}

struct s_sym *lookup(char *s) /*return pointer to symbol table entry for s */
{
	struct s_sym *sp;
   
	for (sp = symtab[hash(s)];sp != NULL; sp = sp->next)
		if (strcmp(s,sp->name) == 0) return(sp);
	return(NULL);
}

void initvar(void)
{
	int i;
	static int init_done = FALSE;

	if (!init_done) {
		for (i=0;i<HASHSIZE;i++) symtab[i] = NULL;
		init_done = TRUE;
	}
}

int defvar(char *s)         /* define variable name s */
{
	struct s_sym *sp;
	int hashval;
   
	if ((sp=lookup(s)) == NULL) {
		sp = (struct s_sym *) malloc(sizeof(struct s_sym));
		if (sp == NULL) return(FALSE);
		sp->value = (struct s_var *) malloc(sizeof (struct s_var));
		if (sp->value == NULL) return(FALSE);
		if ((sp->name=strsave(s)) == NULL) return(FALSE);   
		hashval = hash(sp->name);
		sp->next = symtab[hashval];
		symtab[hashval] = sp;
		sp->value->type = S_INT;
		sp->value->val.ival = 0;
	}
	return(TRUE);
}
 
int setvar(char *s,struct s_var *value)  /* set variable name s * to value val */  
{
	struct s_sym *sp;
   
	if (debug) {
		fprintf(stderr,"setting var %s to ",s);
		if (value->type == S_INT)
			fprintf(stderr,"%d\n",value->val.ival);
		else if (value->type == S_STR)
			fprintf(stderr,"%s\n",value->val.pval);
		else
			fprintf(stderr,"(eh?)\n");
	}
	if ((sp=lookup(s)) == NULL) return(FALSE);
	if (sp->value->type == S_STR) free(sp->value->val.pval);
	sp->value->type = value->type;
	if (value->type == S_STR) {
		if ((sp->value->val.pval=strsave(value->val.pval)) == NULL)
			return(FALSE);   
	}
	else if (value->type == S_INT)
		sp->value->val.ival = value->val.ival;
	else
		return(FALSE);
	return(TRUE);
}

/*
 * getvar obtains the value of variable named in *s.  A value, not a
 * reference is returned.  The client is responsible for freeing the
 * memory assigned to returned string variables.
 */

int getvar(char *s,struct s_var *value)     /* get value of variable s */
{
	struct s_sym *lookup(),*sp;

	if ((sp=lookup(s)) == NULL) {
		/* define if variable doesn't already exist - default to S_INT */
		if (!defvar(s))	return(FALSE);
		value->type = S_INT;
		value->val.ival = 0;
		return(TRUE);
	}
	
	value->type = sp->value->type;
	if (value->type == S_STR) {
        /* always return copy of string */
        value->val.pval = strsave(sp->value->val.pval);
        if (value->val.pval == NULL) return FALSE;
    }
	else if (value->type == S_INT){
		value->val.ival = sp->value->val.ival;
    }
	else {
		return(FALSE);
    }
	return(TRUE);
}
     
   
void dmpvar()         /* dump symbol table entries */
{
	int i;
	struct s_sym *sp;

	fprintf(stderr,"name     type    value\n");
	for (i=0;i<HASHSIZE;i++) {
		sp = symtab[i];
		while (sp != NULL) {
			fprintf(stderr,"%-8s ",sp->name);
			if (sp->value->type == S_INT)
				fprintf(stderr,"(int)   %d\n",sp->value->val.ival);
			else if (sp->value->type == S_STR)
				fprintf(stderr,"(char)  %s\n",sp->value->val.pval);
			else
				fprintf(stderr,"(eh?)   %d\n",sp->value->type);
			sp = sp->next;
		}
	}
}

/* Return VAR pointer, created from type and value */
VAR *symbolise(int type, char *s)
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

LIST* new_list() {
    LIST* elt;
    
    elt = (LIST *) malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_error("out of memory at new_list.");
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
    LIST* elt = new_list();

    elt->var = var;
    elt->next = list->val.lval;
    list->val.lval = elt;
    return list;
}


LIST* append(LIST* list,VAR* var)
{
    LIST *elt,*current;

    elt = new_list();
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

LIST* handle_quote(int token_type,LIST* form)
{
    VAR* quoted_list = NULL;
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
    }
    quoted_list = read_list(S_LIST,'(',')');
    return append(form,insert(quote_type,quoted_list));
}

VAR* read_list(int type,char open, char close)
{
    LIST* form = NULL;
    VAR* var;
    int token_type;

    var = new_var();
    token_type = lexer();
    while (token_type != S_EOE && token_type != S_EOF && token_type != close) {
        switch (token_type) {
            case '(':
                form = append(form,read_list(S_LIST,'(',')'));
                break;
            case '[':
                form = append(form,read_list(S_ARRAY,'[',']'));
                break;
            case S_QUOTE:
            case S_QUASIQUOTE:
            case S_UNQUOTE:
            case S_SPLICE:
                form = handle_quote(token_type,form);
                break;
            default:
                form = append(form,symbolise(token_type,lextok));
        }
        token_type = lexer();
    }
    var->type = type;
    var->val.lval = form;
    return var;
}

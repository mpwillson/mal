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
    VAR *new;

    new = (VAR *) malloc(sizeof(VAR)); /* replace by new_var */
    if (new == NULL) return NULL;
    new->type = type;
    switch (type) {
        case S_INT: 
            new->val.ival = atoi(s);
            break;
        case S_VAR:
        case S_STR:
            new->val.pval = strsave(s);
            break;
            
    }   
    return new;
}


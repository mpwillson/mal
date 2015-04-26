/*
 * 	NAME
 *
 *
 * 	SYNOPSIS
 *
 *
 * 	DESCRIPTION
 *
 *
 * 	NOTES
 *
 *
 * 	MODIFICATION HISTORY
 * 	Mnemonic	Rel	Date	Who
 *
 * 		Written.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mal.h"
#include "printer.h"
#include "reader.h"
#include "env.h"

ENV* new_env(int size, ENV* outer,VAR* binds, VAR* exprs)
{
    ENV* env;
    int i;
    SYM** sym;
    LIST* bind_list, *expr_list;
    VAR* rest;
    LIST nil_elt = {&var_nil,NULL};

    /* Initialise new env instance */
    env = (ENV*) malloc(sizeof(ENV));
    env->size = size;
    env->closure = false;
    env->outer = outer;
    env->sym = (SYM**) malloc(sizeof(SYM*)*size);
    sym = env->sym;
    for (i=0;i<size;i++) {
        *sym++ = NULL;
    }
    /* Run through binds list. Handle & (all subsequent args are bound
       as a list to the following binds symbol).  Where binds exist
       without an arg, binds are set to nil.
    */
    if (binds != NULL) {
        bind_list = binds->val.lval;
        expr_list = exprs->val.lval;
        if (expr_list == NULL) expr_list = &nil_elt;
        while (bind_list != NULL) {
            if (strcmp(bind_list->var->val.pval,"&") == 0) {
                bind_list = bind_list->next;
                rest = new_var();
                rest->type = S_LIST;
                rest->val.lval = (expr_list==&nil_elt?NULL:expr_list);
                env_put(env,bind_list->var->val.pval,rest);
                return env;
            }
            env_put(env,bind_list->var->val.pval,expr_list->var);
            bind_list = bind_list->next;
            expr_list = (expr_list->next==NULL?&nil_elt:expr_list->next);
        }
    }
    return env;
}

int hash(ENV* env,char *s)  
{
	int hashval;
   
	for (hashval=0; *s != '\0'; ) hashval += *s++;
	return (hashval%(env->size));
}

SYM *lookup(ENV* env,char *s)
{
	SYM *sp;
   
	for (sp = env->sym[hash(env,s)];sp != NULL; sp = sp->next) {
		if (strcmp(s,sp->name) == 0) return sp;
    }
	return NULL;
}

ENV* env_put(ENV* env,char *name,VAR* var)
{
	SYM *sp;
	int hashval;
   
	if ((sp=lookup(env,name)) == NULL) {
		sp = (SYM *) malloc(sizeof(SYM));
		if (sp == NULL) return NULL;
        sp->name = strsave(name);
		sp->value = var;
		hashval = hash(env,sp->name);
		sp->next = env->sym[hashval];
		env->sym[hashval] = sp;
	}
    else {
        sp->value = var;
    }
	return env;
}

VAR* env_get(ENV* env,char* name)
{
    SYM* sp;

    if (env == NULL) {
        return NULL;
    }
	else if ((sp=lookup(env,name)) != NULL) {
        return sp->value;
    }
    return env_get(env->outer,name);
}
    
void env_dump(ENV* env)
{
    SYM* sp;
    int i;

    for (i=0;i<env->size;i++) {
        if (env->sym[i] != NULL) {
            sp = env->sym[i];
            while (sp != NULL) {
                printf("[%d] %s: type %d, value: %s\n",
                       i,sp->name,sp->value->type,
                       print_str(sp->value,true,true));
                sp = sp->next;
            }
        }
    }
    /* if (env->outer != NULL) { */
    /*     printf("Enclosed by...\n"); */
    /*     env_dump(env->outer); */
    /* } */
}

void env_free(ENV* env)
{
    SYM* sp,*last_sp;
    int i;
    if (env->closure) return;
    for (i=0;i<env->size;i++) {
        if (env->sym[i] != NULL) {
            while (sp != NULL) {
                sp = env->sym[i];
                last_sp = sp;
                /* VARs may be required for results passed back to
                 * outer environment.  TBD: Handle all this garbage
                 * that is being created */
                /* free(sp->name); */
                /* free_var(sp->value); */
                sp = sp->next;
                free(last_sp);
            }
        }
    }
    free(env->sym);
    free(env);
}

    
            

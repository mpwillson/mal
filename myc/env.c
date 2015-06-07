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
#include "mem.h"

HASH* new_env(int size, HASH* outer,VAR* binds, VAR* exprs)
{
    HASH* env;
    LIST* bind_list = NULL, *expr_list;
    VAR* rest;
    LIST nil_elt = {&var_nil,1,NULL};

    /* Initialise new env instance */
    env = new_hash(size);
    env->outer = outer;
    
    /* Run through binds list. Handle & (all subsequent args are bound
       as a list to the following binds symbol).  Where binds exist
       without an arg, binds are set to nil.
    */
    if (binds != NULL) {
        if (binds->type == S_VECTOR || binds->type == S_LIST) {
            bind_list = seq(binds)->val.lval;
        }
        else {
            throw(mal_error("invalid binding form: %s",
                            print_str(binds,true,true)));
        }
        expr_list = (exprs?exprs->val.lval:NULL);
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
    env_add(env);
    return env;
}

static int hashed(HASH* env,char *s)  
{
	int hashval;
   
	for (hashval=0; *s != '\0'; ) hashval += *s++;
	return (hashval%(env->size));
}

static SYM *lookup(HASH* env,char *s)
{
	SYM *sp;
   
	for (sp = env->sym[hashed(env,s)];sp != NULL; sp = sp->next) {
		if (strcmp(s,sp->name) == 0) return sp;
    }
	return NULL;
}

HASH* env_del(HASH* env,char *name)
{
    SYM* sp, *sp_prev = NULL;
    int hashval;

    hashval = hashed(env,name);
    sp = env->sym[hashval];
    while (sp != NULL) {
        if (strcmp(name,sp->name) == 0) {
            if (!sp_prev) {
                env->sym[hashval] = NULL;
            }
            else {
                sp_prev->next = sp->next;
            }
            free(sp);
            sp = NULL;
        }
        else {
            sp_prev = sp;
            sp = sp->next;
        }
    }
    return env;
}
    
HASH* env_put(HASH* env,char *name,VAR* var)
{
	SYM *sp;
	int hashval;
   
	if ((sp=lookup(env,name)) == NULL) {
		sp = (SYM *) malloc(sizeof(SYM));
		if (sp == NULL) return NULL;
        sp->name = strsave(name);
		sp->value = var;
		hashval = hashed(env,sp->name);
		sp->next = env->sym[hashval];
		env->sym[hashval] = sp;
	}
    else {
        sp->value = var;
    }
	return env;
}

VAR* env_get(HASH* env,char* name)
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

void env_free(HASH* env)
{
    SYM* sp,*last_sp;
    int i;
    
    if (env->closure) return;
    for (i=0;i<env->size;i++) {
        if ((sp=env->sym[i])) {
            while (sp != NULL) {
                sp = env->sym[i];
                last_sp = sp;
                sp = sp->next;
                free(last_sp->name);
                free(last_sp);
            }
        }
    }
    free(env->sym);
    free(env);
}

/* Support for iteration over a hash (including nested hashes).
 * Initialisation returns an iterator, which is passed to env_next.
 * env_next returns the next hash value as a SYM*.
 * Client is responsible for freeing iterator when done.
 */
ITER* env_iter_init(HASH* h)
{
    ITER* iter;

    iter = (ITER*) malloc(sizeof(ITER));
    if (iter == NULL) {
        mal_die("out of memory at env_iter_init");
    }
    iter->hash = h;
    iter->sp = NULL;
    iter->index = 0;
    return iter;
}

SYM* env_next(ITER* iter)
{
    if (iter->sp == NULL) {
        if (iter->index < iter->hash->size) {
            iter->sp = iter->hash->sym[iter->index];
            iter->index++;
            if (!iter->sp) return env_next(iter);
        }
        else {
            return NULL;
        }
    }
    else {
        iter->sp = iter->sp->next;
        if (!iter->sp) return env_next(iter);
    }
    return iter->sp;
}

/* Example of hash iterator usage */
void env_dump(HASH* env)
{
    SYM* sp;
    ITER* iter;
    
    iter = env_iter_init(env);
    while ((sp = env_next(iter)) != NULL) {
        printf("%-24s%s\n",
               sp->name,print_str(sp->value,true,true));
    }
    free(iter);
}

        
            
    

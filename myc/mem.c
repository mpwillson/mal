#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "mal.h"
#include "env.h"
#include "mem.h"
#include "core.h"
#include "printer.h"

struct s_mem {
    int nvars;
    int nelts;
    int nfns;
    int nvecs;
};
typedef struct s_mem MEM;

struct s_envlist {
    HASH* env;
    struct s_envlist* next;
};
typedef struct s_envlist ENVLIST;

static MEM mem_inuse = {0,0,0,0};
static LIST* allocated = NULL;
static ENVLIST* envs = NULL;


void print_mem(void)
{
    int nenvs = 0;
    ENVLIST* e;

    e = envs;
    while (e) {
        nenvs++;
        e = e->next;
    }
    
    fprintf(stderr,"%-10s %10d\n%-10s %10d\n%-10s %10d\n%-10s %10d\n"
            "%-10s %10d\n%-10s %10d\n",
            "vars:",mem_inuse.nvars,
            "elts:",mem_inuse.nelts,
            "fns:",mem_inuse.nfns,
            "vecs:",mem_inuse.nvecs,
            "allocated:",count(allocated),
            "envs:",nenvs);
}

void env_add(HASH* hash)
{
    ENVLIST* env;

    env = (ENVLIST*) malloc(sizeof(ENVLIST));
    if (env == NULL) mal_die("out of memory at env_add");
    env->next = envs;
    env->env = hash;
    envs = env;
    /* VAR* var = new_var(),*env_var; */

    /* var->type = S_HASHMAP; */
    /* var->val.hval = hash; */
    /* env_var = cons(var,envs); */
    /* envs = env_var->val.lval; */
    return;
}

char *strsave(char *s)
{
	char *p;

	p = (char *) malloc(strlen(s)+1);
	if (p != NULL) 	strcpy(p,s);
	return p;
}

void* mal_malloc(size_t size)
{
    void* ptr;

    ptr = malloc(size);
    /* if ptr is nil, invoke gc */
    return ptr;
}

HASH* new_hash(int size)
{
    HASH* hash;
    SYM** sym;
    int i;

    hash = (HASH*) malloc(sizeof(HASH));
    if (!hash) mal_die("out of memory in new_hash");
    hash->size = size;
    hash->closure = false;
    hash->outer = NULL;
    hash->sym = (SYM**) malloc(sizeof(SYM*)*size);
    if (!hash->sym) mal_die("out of memory in new_hash (sym)");
    sym = hash->sym;
    for (i=0;i<size;i++) {
        *sym++ = NULL;
    }
    return hash;
}

LIST* new_elt() {
    LIST* elt;
    
    elt = (LIST *) malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_die("out of memory at new_elt.");
    }
    mem_inuse.nelts++;
    elt->var = NULL;
    elt->next = NULL;
    return elt;
}

VAR* new_var() {
    VAR* var;
    LIST* elt = new_elt();
    
    var = (VAR *) mal_malloc(sizeof(VAR));
    if (var == NULL)  {
        mal_die("out of memory at new_var.");
    }
    mem_inuse.nvars++;
    var->type = S_UNDEF;
    var->val.lval = NULL;
    var->marked = false;
    elt->var = var;
    elt->next = allocated;
    allocated = elt;
    return var;
}
      
FN* new_fn()
{
    FN* fn;

    fn = (FN*) mal_malloc(sizeof(FN));
    if (fn == NULL) {
        mal_die("out of memory at new_fn");
    }
    mem_inuse.nfns++;
    fn->args = NULL;
    fn->forms = NULL;
    fn->env = NULL;
    return fn;
}

VEC* new_vec(int size)
{
    VEC* vec;

    vec = (VEC*) mal_malloc(sizeof(VEC));
    if (vec) {
        vec->size = size;
        vec->vector = (VAR**) malloc(sizeof(VEC*)*size);
    }
    if (!vec && !vec->vector) mal_die("out of memory at new_vec");
    mem_inuse.nvecs++;
    return vec;
}

void free_elts(LIST* list)
{
    LIST* elt, *last_elt;

    elt = list;
    while (elt) {
        last_elt = elt;
        elt = elt->next;
        if (!last_elt->var->marked) {
            free(last_elt);
            mem_inuse.nelts--;
        }
        else {
            return;
        }
    }
}

void free_var(VAR* var)
{
    printf("Freeing type: %d; %s\n", var->type,print_str(var,false,true));
    switch (var->type) {
        case S_STR:
        case S_SYM:
        case S_KEYWORD:
        case S_COMMENT:
            free(var->val.pval);
            break;
        case S_ROOT:
        case S_LIST:
            free_elts(var->val.lval);
            break;
        case S_VECTOR:
            free(var->val.vval->vector);
            free(var->val.vval);
            mem_inuse.nvecs--;
            break;
        case S_HASHMAP:
            env_free(var->val.hval);
            break;
        case S_FN:
        case S_MACRO:
            free(var->val.fval);
            mem_inuse.nfns--;
            break;
    }
    free(var);
    mem_inuse.nvars--;
}

void mark_var(VAR* var)
{
    LIST* elt;
    ITER* iter;
    SYM* sp;
    int i;
    
    var->marked = true;
    switch (var->type) {
        case S_ROOT:
        case S_LIST:
            elt = var->val.lval;
            while (elt) {
                mark_var(elt->var);
                elt = elt->next;
            }
            break;
        case S_VECTOR:
            for (i = 0; i<var->val.vval->size; i++) {
                mark_var(var->val.vval->vector[i]);
            }
            break;
        case S_FN:
        case S_MACRO:
            mark_var(var->val.fval->args);
            mark_var(var->val.fval->forms);
            break;
        case S_HASHMAP:
            iter = env_iter_init(var->val.hval);
            while ((sp=env_next(iter))) {
                mark_var(sp->value);
            }
            break;
    }
    return;
}

bool walk_env(HASH* env)
{
    ITER* iter;
    SYM* sp;

    if (env->closure) {
        iter = env_iter_init(env);
        while ((sp = env_next(iter))) {
            mark_var(sp->value);
        }
        free(iter);
        return true;
    }
    else {
        env_free(env);
    }
    return false;
}

void gc(void)
{
    LIST* elt, **elt_ptr;
    ENVLIST *e,**ep;
    int nmarked = 0, nunmarked = 0, nundef = 0;
    
    /* Mark all allocated VARs as unseen.*/
    elt = allocated;
    while (elt) {
        elt->var->marked = false;
        elt = elt->next;
    }
    /* Run through envs. For envs that are "closures", mark all VARs
     * that can be reached. For envs that are not closures, free them.
     */
    e = envs;
    ep = &envs;
    while (e) {
        if (!walk_env(e->env)) {
            printf("env freed.\n");
            *ep = e->next;
            free(e);
            e = *ep;
            mem_inuse.nelts--;
        }
        else {
            ep = &((*ep)->next);
            e = e->next;
        }
    }

    elt = allocated;
    while (elt) {
        if (elt->var->marked) {
            /* nmarked++; */
            printf("Marked: [%d] %s\n",nmarked++,print_str(elt->var,false,true));
        }
        else {
            /* nunmarked++; */
            printf("Unmarked: [%d] %s\n",nunmarked++,print_str(elt->var,false,true));
        }
        if (elt->var->type == S_UNDEF) nundef++;
        elt = elt->next;
    }
    printf("VARS marked: %d (undefined: %d)\n",nmarked,nundef);
    /* if (true) return; */

    /* Run through allocated, freeing everything not marked.
     * Update allocated stats.
     */
    elt = allocated;
    elt_ptr = &allocated;
    nmarked = 0;
    nunmarked = 0;
    while (elt) {
        if (!elt->var->marked) {
            free_var(elt->var);
            *elt_ptr = elt->next;
            free(elt);
            elt = *elt_ptr;
            nunmarked++;
            mem_inuse.nelts--;
        }
        else {
            elt_ptr = &((*elt_ptr)->next);
            elt = elt->next;
            nmarked++;
        }
    }
    printf("Freed: %d; Current: %d\n",nunmarked,nmarked);

}

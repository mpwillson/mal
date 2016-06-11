#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "mal.h"
#include "env.h"
#include "mem.h"
#include "core.h"
#include "printer.h"

#define DEBUG 0

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
static LIST* active_vars = NULL; /* For VARs that are active, but not
                                  * part of an env */

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
    return;
}

char *strsave(char *s)
{
	char *p;

	p = (char *) mal_malloc(strlen(s)+1);
	if (p != NULL) 	strcpy(p,s);
	return p;
}

void* mal_malloc(size_t size)
{
    void* ptr;

    ptr = malloc(size);
    return ptr;
}

HASH* new_hash(int size)
{
    HASH* hash;
    SYM** sym;
    int i;

    hash = (HASH*) mal_malloc(sizeof(HASH));
    if (!hash) mal_die("out of memory in new_hash");
    hash->size = size;
    hash->closure = false;
    hash->outer = NULL;
    hash->sym = (SYM**) mal_malloc(sizeof(SYM*)*size);
    if (!hash->sym) mal_die("out of memory in new_hash (sym)");
    sym = hash->sym;
    for (i=0;i<size;i++) {
        *sym++ = NULL;
    }
    return hash;
}

LIST* new_elt() {
    LIST* elt;
    
    elt = (LIST *) mal_malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_die("out of memory at new_elt.");
    }
    mem_inuse.nelts++;
    elt->var = NULL;
    elt->refs = 0;
    elt->next = NULL;
    return elt;
}

LIST* ref_elt(LIST* elt)
{
    if (elt) elt->refs++;
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
    var->meta = NULL;
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

void add_active(VAR* var)
{
    LIST* elt = new_elt();

    elt->var = var;
    elt->next = active_vars;
    active_vars = elt;
}

void del_active(int n)
{
    LIST* elt;
    
    while (n--) {
        elt = active_vars->next;
        free(active_vars);
        active_vars = elt;
    }
    return;
}

void free_elts(LIST* list)
{
    LIST* elt, *last_elt;

    elt = list;
    while (elt) {
        last_elt = elt;
        elt = elt->next;
        if (last_elt->refs <= 1) {
            free(last_elt);
            mem_inuse.nelts--;
        }
        else {
            last_elt->refs--;
            return;
        }
    }
}

void free_var(VAR* var)
{
    if (var->marked) return;
    
    if (DEBUG) {
        printf("Freeing type: %d; %s\n", var->type,print_str(var,false,true));
    }
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
            free_var(var->val.fval->args);
            env_free(var->val.fval->env);
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

    if (var->marked) return; /* been here before */
    
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
            free(iter);
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
    /* Mark active vars */
    elt = active_vars;
    while (elt) {
        mark_var(elt->var);
        elt = elt->next;
    }
    /* Run through envs. For envs that are "closures", mark all VARs
     * that can be reached. For envs that are not closures, free them.
     */
    e = envs;
    ep = &envs;
    while (e) {
        if (!walk_env(e->env)) {
            *ep = e->next;
            free(e);
            e = *ep;
        }
        else {
            ep = &((*ep)->next);
            e = e->next;
        }
    }

    elt = allocated;
    while (elt) {
        if (elt->var->marked) {
            nmarked++;
            if (DEBUG) {
                printf("Marked: [%d] %s\n",nmarked,
                       print_str(elt->var,false,true));
            }
        }
        else {
            nunmarked++;
            if (DEBUG) {
                printf("Unmarked: [%d] %s\n",
                       nunmarked,print_str(elt->var,false,true));
            }
        }
        if (elt->var->type == S_UNDEF) nundef++;
        elt = elt->next;
    }

    /* Run through allocated, freeing everything not marked.
     * Update allocated stats.
     */
    elt = allocated;
    elt_ptr = &allocated;
    nmarked = 0;
    nunmarked = 0;
    while (elt) {
        if (!elt->var->marked) {
            if (DEBUG) {
                printf("Freeing: %s @ x%x -> x%x\n",
                       print_str(elt->var,true,true),
                       (unsigned int)elt->var,(unsigned int)elt->var->val.pval);
            }
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
    if (DEBUG) printf("gc: freed: %d; current: %d\n",nunmarked,nmarked); 

}

void check_for_gc(int n)
{
    int inuse;
    
    if (mem_inuse.nvars > n && n > 0) {
        inuse = mem_inuse.nvars;
        gc();
        printf("GC: %d vars were in-use; freed: %d\n",inuse,inuse-mem_inuse.nvars);
    }
}

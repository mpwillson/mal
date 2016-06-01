#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <readline/readline.h>

#include "mal.h"
#include "env.h"
#include "core.h"
#include "printer.h"
#include "mem.h"

struct s_builtin {
    char* name;
    BUILTIN fn;
};

int count(LIST* list)
{
    int len = 0;
    LIST* elt = list;
    
    while (elt != NULL) {
        len++;
        elt = elt->next;
    }
    return len;
}

VAR* b_count(LIST* list)
{
    VAR* var = new_var();

    var->type = S_INT;
    var->val.ival = 0;
    if (list) {
        if (list->var->type == S_VECTOR) {
            var->val.ival = list->var->val.vval->size;
        }
        else if (islist(list->var->type)) {
            var->val.ival = count(list->var->val.lval);
        }
    }
    return var;
}

VAR* b_listp(LIST* list)
{
    if (list->var->type == S_LIST) {
        return &var_true;
    }
    else {
        return &var_false;
    }
}

VAR* b_emptyp(LIST* list)
{
    if (list->var->type == S_VECTOR) {
        return (list->var->val.vval->size==0?&var_true:&var_false);
    }
    else if (islist(list->var->type) && list->var->val.lval == NULL) {
        return &var_true;
    }
    else {
        return &var_false;
    }
}

/* forward declaration */
bool list_equalp(LIST*,LIST*);
bool vec_equalp(VEC*,VEC*);

VAR* var_equalp(VAR* v1, VAR* v2)
{
    bool eq;

    /* all list types are equal in mal */
    if (!(islist(v1->type) && islist(v2->type)) &&
         v1->type != v2->type) return &var_false;
    switch (v1->type) {
        case S_INT:
            eq = v1->val.ival == v2->val.ival;
            break;
        case S_REAL:
            eq = v1->val.rval == v2->val.rval;
            break;
        case S_STR:
        case S_SYM:
        case S_KEYWORD:
            eq = (strcmp(v1->val.pval,v2->val.pval)==0);
            break;
        case S_FN:
            eq = (v1->val.fval == v2->val.fval);
            break;
        case S_BUILTIN:
            eq = (v1->val.bval == v2->val.bval);
            break;
        case S_VECTOR:
            if (v2->type == S_VECTOR) {
                eq = vec_equalp(v1->val.vval,v2->val.vval);
            }
            else {
                eq = list_equalp(seq(v1)->val.lval,v2->val.lval);
            }
            break;
        case S_LIST:
        case S_HASHMAP:
        case S_ROOT:
            eq = list_equalp(v1->val.lval,seq(v2)->val.lval);
            break;
        case S_NIL:
        case S_FALSE:
        case S_TRUE:
            eq = true;
            break;
        default:
            eq = false;
    }
    return (eq?&var_true:&var_false);
}

bool list_equalp(LIST* l1, LIST* l2)
{
    LIST* e1,*e2;

    e1 = l1;
    e2 = l2;
    while (e1 != NULL && e2 != NULL) {
        if (!var_equalp(e1->var,e2->var)) {
            return false;
        }
        e1 = e1->next;
        e2 = e2->next;
    }
    return (e1 == NULL && e2 == NULL);
}

bool vec_equalp(VEC* v1,VEC* v2)
{
    int i;
    
    if (v1->size != v2->size) return false;
    for (i=0;i<v1->size;i++) {
        if (!var_equalp(v1->vector[i],v2->vector[i])) return false;
    }
    return true;
}

VAR* b_equalp(LIST* list)
{
    VAR* v1= NULL, *v2 = NULL;

    if (list == NULL) return &var_false;
    v1 = list->var;
    if (list->next == NULL) return &var_false;
    v2 = list->next->var;
    return var_equalp(v1,v2);
}

VAR* arith(char type,LIST* list)
{
    LIST* elt;
    VAR* result = new_var();
    int len;
    
    elt = list;
    result->type = S_INT;
    result->val.ival = 0;
    if (type == '*') {
        result->val.ival = 1;
    }
    else if ((type == '-' || type == '/') && list != NULL) {
        len = count(list);
        if (len != 1) {
            result->val.ival = list->var->val.ival;
            elt = list->next;
        }
    }
    while (elt != NULL) {
        if (elt->var->type != S_INT) throw(mal_error("integer expected"));
        switch (type) {
            case '+':
                result->val.ival += elt->var->val.ival;
                break;
           case '-':
                result->val.ival -= elt->var->val.ival;
                break;
           case '*':
                result->val.ival *= elt->var->val.ival;
                break;
           case '/':
                result->val.ival = result->val.ival / elt->var->val.ival;
                break;
        }
        elt = elt->next;
    }
    return result;
}

VAR* b_plus(LIST* list)
{
    /* VAR* var; */
    
    return arith('+',list);
}

    /* CODE to allow promotion of ints to reals. */
    /* var = new_var(); */
    /* if (sum->type == S_FLOAT) { */
    /*     switch (add->type) { */
    /*         case S_INT: */
    /*             sum->val.fval += add->val.ival; */
    /*             break; */
    /*         case S_FLOAT: */
    /*             sum->val.fval += add->val.fval; */
    /*             break; */
    /*     } */
    /* } */
    /* else { /\*assume sum type is S_INT *\/ */
    /*     if (add->type == S_FLOAT) { */
    /*         sum->type = S_FLOAT; */
    /*         sum->val.fval = sum->val.ival + add->val.fval; */
    /*     } */
    /*     else { */
    /*         sum->val.ival += add->val.ival; */
    /*     } */
    /* } */
    /* return sum; */

VAR* b_mul(LIST* list)
{
    return arith('*',list);
}

VAR* b_div(LIST* list)
{
    return arith('/',list);
}

VAR* b_minus(LIST* list)
{
    return arith('-',list);
}

VAR* b_list(LIST* list)
{
    return list2var(list);
}

#define S_LTEQ 1
#define S_GTEQ 2

VAR* compare(char type,LIST* list)
{
    LIST* elt;
    int current = INT_MIN;
    bool satisfied = true;

    elt = list;
    if (type == '<' || type == S_LTEQ) {
        current = INT_MIN;
    }
    else {
        current = INT_MAX;
    }
    while (elt != NULL) {
        if (elt->var->type == S_INT) {
            switch (type) {
                case '<':
                    satisfied = (current < elt->var->val.ival);
                    break;
                case '>':
                    satisfied = (current > elt->var->val.ival);
                    break;
                case S_LTEQ:
                    satisfied = (current <= elt->var->val.ival);
                    break;
                case S_GTEQ:
                    satisfied = (current >= elt->var->val.ival);
                    break;
            }
            if (!satisfied) return &var_false;
            current = elt->var->val.ival;
            elt = elt->next;
        }
    }
    return &var_true;
}
                    

VAR* b_lessthanp(LIST* list)
{
    return compare('<',list);
}

VAR* b_lessthaneqp(LIST* list)
{
    return compare(S_LTEQ,list);
}

VAR* b_greaterthanp(LIST* list)
{
    return compare('>',list);
}

VAR* b_greaterthaneqp(LIST* list)
{
    return compare(S_GTEQ,list);
}


VAR* pr_str(LIST* list,bool readable)
{
    LIST* elt;
    VAR* var = new_var();
    bool top_level = true;
    char* buffer = NULL;

    elt = list;
    while (elt != NULL) {
        buffer = print_str(elt->var,readable,top_level);
        top_level = false;
        if (readable) strcat(buffer," ");
        elt = elt->next;
    }
    if (readable && buffer) buffer[strlen(buffer)-1] = '\0';
    var->type = S_STR;
    var->val.pval = (buffer?strsave(buffer):strsave(""));
    return var;
}

VAR* b_pr_str(LIST* list)
{
    return pr_str(list,true);
}

VAR* b_str(LIST* list)
{
    return pr_str(list,false);
}

VAR* prn(LIST* list,bool readable)
{
    LIST* elt;
    bool top_level = true;
    char* buffer = NULL;

    elt = list;
    while (elt != NULL) {
        buffer = print_str(elt->var,readable,top_level);
        top_level= false;
        strcat(buffer," ");
        elt = elt->next;
    }
    if (buffer) {
        buffer[strlen(buffer)-1] = '\0';
        printf("%s\n",buffer);
    }
    else {
        printf("\n");
    }
    return &var_nil;
}

VAR* b_prn(LIST* list)
{
    return prn(list,true);
}

VAR* b_println(LIST* list)
{
    return prn(list,false);
}

VAR* b_read_string(LIST* list)
{
    if (list == NULL) return &var_nil;
    if (list->var->type == S_STR) {
        return repl_read(list->var->val.pval);
    }
    return &var_nil;
}

#define BUFSZ 1024

VAR* b_slurp(LIST* list)
{
    struct stat statbuf;
    FILE *in;
    char buf[BUFSZ+1], *slurp_buf, *fn;
    int nitems, offset;
    bool eof;
    VAR* var;

    if (list == NULL) return &var_nil;
    if (list->var->type == S_STR) {
        fn = list->var->val.pval;
        in = fopen(fn,"r");
        if (in == NULL) {
            throw(mal_error("unable to open file: '%s'",fn));
            return &error;
        }
        if (fstat(fileno(in),&statbuf) != 0) {
            throw(mal_error("fstat failed for file '%s'",fn));
            fclose(in);
            return &error;
        }
        /* is this a regular file? */
        if (!S_ISREG(statbuf.st_mode)) {
            throw(mal_error("file '%s' is not a regular file",fn));
            fclose(in);
            return &error;
        }
        /* acquire memory to hold file contents */
        slurp_buf = (char *) malloc(statbuf.st_size+1);
        if (slurp_buf == NULL) {
            throw(mal_error("no memory for slurp buffer"));
            fclose(in);
            return &error;
        }
        /* read in contents of file */
        eof = false;
        offset = 0;
        while (!eof) {
            nitems = fread(buf,sizeof(char),BUFSZ,in);
            eof = (nitems != BUFSZ);
            memcpy(slurp_buf+offset,buf,nitems);
            offset += nitems;
            if (offset > statbuf.st_size) {
                throw(mal_error("internal error - buffer overflow"));
                free(slurp_buf);
                fclose(in);
                return &error;
            }
        }
        slurp_buf[statbuf.st_size] = '\0';
        var = new_var();
        var->type = S_STR;
        var->val.pval = slurp_buf;
        fclose(in);
        return var;
    }
    return &var_nil;   
}
   
VAR* b_eval(LIST* list)
{
    return eval(list->var,ns_get());
}

VAR* cons(VAR* var,LIST* list)
{
    LIST* elt = new_elt();
    VAR* new = new_var();

    elt->var = var;
    elt->next = ref_elt(list);
    new->type = S_LIST;
    new->val.lval = ref_elt(elt);
    return new;
}

VAR* b_cons(LIST* list)
{
    VAR* var;
    LIST* elt;

    if (list ==  NULL) return &var_nil;
    var = list->var;
    elt = list->next;
    if (elt && islist(elt->var->type)) {
            return cons(var,seq(elt->var)->val.lval);
    }
    else if (!elt) {
        return cons(var,NULL);
    }
    throw(mal_error("cons target not a list"));
    return &error;
}

HASH* copy_hashmap(HASH* hash)
{
    HASH* new;
    SYM* sp;
    ITER* iter;
    
    new = new_env(hash->size,NULL,NULL,NULL);
    iter = env_iter_init(hash);
    while ((sp=env_next(iter))) {
        env_put(new,sp->name,sp->value);
    }
    free(iter);
    return new;
}

// FIXME
VEC* copy_vector(VEC* vec)
{
    return vec;
}

VAR* copy_var(VAR* var)
{
    VAR* new = new_var();

    new->type = var->type;
    switch (var->type) {
        case S_ROOT:
        case S_LIST:
            new->val.lval = copy_list(var->val.lval);
            break;
        case S_VECTOR:
            new->val.vval = copy_vector(var->val.vval);
            break;
        case S_HASHMAP:
            new->val.hval = copy_hashmap(var->val.hval);
            break;
        case S_STR:
        case S_KEYWORD:
        case S_SYM:
            new->val.pval = strsave(var->val.pval);
            break;
        default:
            new->val = var->val;
    }
    return new;
}
            
LIST* deep_copy_list(LIST* list)
{
    LIST* new_list = NULL;
    LIST* elt;
    VAR* var;

    elt = list;
    while (elt) {
        var = copy_var(elt->var);
        new_list = append(new_list,var);
        elt = elt->next;
    }
    return new_list;
}

/* Return new list, created from argument.  Vars that point to data
 * (e.g. strings) continue to point to the original data. */
LIST* copy_list(LIST* list)
{
    LIST* new_list = NULL;
    LIST* elt;
    VAR* var;

    elt = list;
    while (elt) {
        var = new_var();
        var->type = elt->var->type;
        var->val = elt->var->val;
        new_list = append(new_list,var);
        elt = elt->next;
    }
    return new_list;
}

LIST* concat(LIST* l1, LIST* l2)
{
    LIST* elt;

    elt = l1;
    while (elt->next) {
        elt = elt->next;
    }
    elt->next = ref_elt(l2);
    return l1;
    
}

VAR* b_concat(LIST* list)
{
    LIST* new = NULL;
    LIST* elt;
    
    elt = list;
    while (elt) {
        if (islist(elt->var->type)) {
            if (new) {
                /* new = concat(new,copy_list(seq(elt->var)->val.lval)); */
                new = concat(new,seq(elt->var)->val.lval);
            }
            else {
                /* new = copy_list(seq(elt->var)->val.lval); */
                new = seq(elt->var)->val.lval;
            }
        }
        elt = elt->next;
    }
    return list2var(new);
}

VAR* b_first(LIST *list)
{
    VAR* var = NULL;
    
    if (list) var = first(list->var);
    return (var?var:&var_nil);
}

VAR* b_second(LIST *list)
{
    VAR* var = NULL;
    
    if (list) var = second(list->var);
    return (var?var:&var_nil);
}

VAR* b_rest(LIST *list)
{
    VAR* var = NULL;
    
    if (list) var = rest(list->var);
    return (var?var:&empty_list);
}

VAR* b_nth(LIST* list)
{
    VAR* vlist;
    VAR* count;
    LIST* elt = NULL;
    int n;

    vlist = list->var;
    count = (list->next?list->next->var:&var_nil);
    if (islist(vlist->type) && count->type == S_INT) {
        if (vlist->type == S_VECTOR &&
            count->val.ival >= 0 &&
            count->val.ival < vlist->val.vval->size ) {
            return vlist->val.vval->vector[count->val.ival];
        }
        else if (vlist->type != S_VECTOR) {
            elt = vlist->val.lval;
            n = count->val.ival;
            while (elt && n > 0) {
                n--;
                elt = elt->next;
            }
        }
    }
    if (!elt) {
        throw(mal_error("nth index out of range"));
    }
    return elt->var;
}

VAR* b_throw(LIST* list)
{
    throw((list?list->var:&var_nil));
    return &error;
}

VAR* b_apply(LIST* list)
{
    VAR* apply_seq, *apply_args, *apply_fn;
    FN* fn;
    LIST* apply_list;
    
    if (!list) return &var_nil;
    apply_fn = list->var;
    list = list->next;
    apply_args = but_last(list);
    apply_seq = last(list);
    if (!islist(apply_seq->type)) {
        throw(mal_error("apply form must specify seq"));
    }
    else {
        if (apply_args->val.lval) {
            apply_list = concat(apply_args->val.lval,seq(apply_seq)->val.lval);
        }
        else {
            apply_list = seq(apply_seq)->val.lval;
        }
        if (apply_fn->type == S_BUILTIN) {
            return apply_fn->val.bval(apply_list);
        }
        else if (apply_fn->type == S_FN) {
            fn = apply_fn->val.fval;
            return eval(fn->forms,new_env(37,fn->env,fn->args,
                                          list2var(apply_list)));
        }
        else {
            throw(mal_error("apply object not callable"));
        }
    }
    return &var_nil;
}

VAR* b_map(LIST* list)
{
    VAR* map_fn;
    LIST* new_list = NULL, *mapped_list = NULL,*elt;   

    if (!list) return &empty_list;
    map_fn = list->var;
    elt = list->next;
    if (elt && islist(elt->var->type)) {
        elt = seq(elt->var)->val.lval;
        while (elt) {
            new_list = append(append(NULL,map_fn),
                              list2var(append(NULL,elt->var)));
            mapped_list = append(mapped_list,b_apply(new_list));
            elt = elt->next;
        }
    }
    return list2var(mapped_list);
}

VAR* b_nil(LIST* list)
{
    return (list&&list->var->type == S_NIL?&var_true:&var_false);
}

VAR* b_true(LIST* list)
{
    return (list&&list->var->type == S_TRUE?&var_true:&var_false);
}

VAR* b_false(LIST* list)
{
    return (list&&list->var->type == S_FALSE?&var_true:&var_false);
}

VAR* b_is_symbol(LIST* list)
{
    return (list&&list->var->type == S_SYM?&var_true:&var_false);
}

VAR* b_symbol(LIST* list)
{
    VAR* var = &var_nil;
    
    if (list && list->var->type == S_STR) {
        var = new_var();
        var->type = S_SYM;
        var->val.pval = strsave(list->var->val.pval);
    }
    return var;
}

VAR* b_is_keyword(LIST* list)
{
    return (list&&list->var->type == S_KEYWORD?&var_true:&var_false);
}

VAR* b_keyword(LIST* list)
{
    VAR* var = &var_nil;
    
    if (list) {
        if (list->var->type == S_STR) {
            var = new_var();
            var->type = S_KEYWORD;
            var->val.pval = strsave(list->var->val.pval);
        }
        else if (list->var->type == S_KEYWORD) {
            var = list->var;
        }
    }
    return var;
}

VAR* b_is_vector(LIST* list)
{
    return (list&&list->var->type == S_VECTOR?&var_true:&var_false);
}

VAR* b_vector(LIST* list)
{
    VAR* var = new_var();

    var->type = S_VECTOR;
    var->val.vval = mkvector(list);
    return var;
}

VAR* b_print_env(LIST* list)
{
    env_dump(ns_get());
    return &var_nil;
}

VAR* b_hash_map(LIST* list)
{
    VAR* var;
    HASH* hash;
    
    hash = mkhashmap(list);
    if (!hash) throw(mal_error("odd number of forms for hashmap"));
    var = new_var();
    var->type = S_HASHMAP;
    var->val.hval = hash;
    return var;
}

VAR* b_is_hash_map(LIST* list)
{
    return (list&&list->var->type == S_HASHMAP?&var_true:&var_false);
}

VAR* b_get(LIST* list)
{
    VAR* var;

    if (list && list->var->type == S_HASHMAP && list->next &&
        (list->next->var->type == S_STR||list->next->var->type == S_KEYWORD)) {
        var = env_get(list->var->val.hval,
                      print_str(list->next->var,false,true));
        return (var?var:&var_nil);
    }
return &var_nil;
}

VAR* b_assoc(LIST* list)
{
    LIST* elt;
    VAR* var = &var_nil;
    HASH* hash, *new_hash;
    int size;

    if (list && list->var->type == S_HASHMAP) {
        hash = list->var->val.hval;
        elt = list->next;
        size = count(elt);
        if (size%2 != 0) throw(mal_error("assoc has odd number of forms"));
        new_hash = copy_hashmap(hash);
        while (elt) {
            env_put(new_hash,print_str(elt->var,false,true),elt->next->var);
            elt = elt->next->next;
        }
        var = new_var();
        var->type = S_HASHMAP;
        var->val.hval = new_hash;
    }
    return var;
}

VAR* b_dissoc(LIST* list)
{
    LIST* elt;
    VAR* var = &var_nil;
    HASH* hash, *new_hash;

    if (list && list->var->type == S_HASHMAP) {
        hash = list->var->val.hval;
        elt = list->next;
        new_hash = copy_hashmap(hash);
        while (elt) {
            env_del(new_hash,print_str(elt->var,false,true));
            elt = elt->next;
        }
        var = new_var();
        var->type = S_HASHMAP;
        var->val.hval = new_hash;
    }
    return var;
}

VAR* b_contains(LIST* list)
{
    VAR* var = NULL;
    
    if (list && list->var->type == S_HASHMAP && list->next &&
        isstr(list->next->var->type)) {
        var = env_get(list->var->val.hval,
                      print_str(list->next->var,false,true));
    }
    return (var?&var_true:&var_false);
}

VAR* b_keys(LIST* list)
{
    LIST* new_list = NULL;
    SYM* sp;
    ITER* iter;
    VAR* var = &empty_list;
    
    if (list && list->var->type == S_HASHMAP) {
        iter = env_iter_init(list->var->val.hval);
        while ((sp=env_next(iter)) != NULL) {
            var = str2var(sp->name);
            new_list = append(new_list,var);
        }
        free(iter);
        var = list2var(new_list);
    }
    return var;
}

VAR* b_vals(LIST* list)
{
    LIST* new_list = NULL;
    SYM* sp;
    ITER* iter;
    VAR* var = &empty_list;
    
    if (list && list->var->type == S_HASHMAP) {
        iter = env_iter_init(list->var->val.hval);
        while ((sp=env_next(iter)) != NULL) {
            new_list = append(new_list,sp->value);
        }
        free(iter);
        var = list2var(new_list);
    }
    return var;
}

VAR* b_is_seq(LIST* list)
{
    return ((list && (list->var->type==S_LIST||list->var->type==S_VECTOR))?
            &var_true:&var_false);
}

VAR* b_print_mem(LIST* list)
{
    print_mem();
    return &var_nil;
}

VAR* b_gc(LIST* list)
{
    gc();
    return &var_nil;
}

VAR* b_readline(LIST* list)
{
    bool at_eof;
    char prompt[73];
    char* buffer;
    VAR* var;
    
    if (list&&list->var->type==S_STR) {
        strncpy(prompt,list->var->val.pval,72);
    }
    else {
        prompt[0] = '\0';
    }
    buffer = readline(prompt);
    at_eof = feof(stdin) || buffer == NULL;
    if (at_eof) return &var_nil;
    var = new_var();
    var->type = S_STR;
    var->val.pval = buffer; /* OK? */
    return var;
}

/* atom support (I'm sure this was originally in StepA, but now
 * is in Step 6) */

VAR* b_atom(LIST* list)
{
    VAR* var;
    
    if (list) {
        var = new_var();
        var->type = S_ATOM;
        var->val.var = list->var;
        return var;
    }
    return &var_nil;
}

VAR* b_is_atom(LIST* list)
{
    return (list && (list->var->type==S_ATOM))?&var_true:&var_false;
}

VAR* b_deref(LIST* list)
{
    return (list && (list->var->type == S_ATOM))?list->var->val.var:&var_nil;
}

VAR* b_reset(LIST* list)
{
    if (list && list->var->type == S_ATOM && list->next) {
        list->var->val.var = list->next->var;
        return list->var;
    }
    return &var_nil;
}

VAR* b_swap(LIST* list)
{
    VAR* atom;
    FN* fn;
    LIST* arg_list;

    if (list && list->var->type == S_ATOM) {
        atom = list->var;
        if (list->next && list->next->var->type == S_FN) {
            fn = list->next->var->val.fval;
            arg_list = concat(append(NULL,atom->val.var),list->next->next);
            atom->val.var = eval(fn->forms,new_env(37,fn->env,fn->args,
                                                   list2var(arg_list)));
            return atom->val.var;
        }
    }
    return &var_nil;
}

/* add meta data at the var level. with-meta returns new var, with
 * meta added. */
VAR* b_with_meta(LIST* list)
{
    VAR* var;
    
    if (list && list->next && list->next->var->type == S_HASHMAP) {
        var = copy_var(list->var);
        var->meta = list->next->var->val.hval;
        return var;
    }
    return &var_nil;
}

VAR* b_meta(LIST* list)
{
    VAR* var;
    
    if (list && list->var->meta) {
        var = new_var();
        var->type = S_HASHMAP;
        var->val.hval = list->var->meta;
        return var;
    }
    return &var_nil;
}

VAR* b_time_ms(LIST* list)
{
    VAR* var;
    struct timeval tv;

    if (gettimeofday(&tv,NULL) == 0) {
        var = new_var();
        var->type = S_INT;
        var->val.ival = (long int) (tv.tv_sec*1000+tv.tv_usec/1000.0);
        return var;
    }
    return &var_nil;
}

VAR* b_is_string(LIST* list)
{
    if (list && list->var->type == S_STR)
        return &var_true;
    else
        return &var_false;
}

    
struct s_builtin core_fn[] =
{
    {"+",b_plus},
    {"-",b_minus},
    {"*",b_mul},
    {"/",b_div},
    {"list",b_list},
    {"count",b_count},
    {"list?",b_listp},
    {"empty?",b_emptyp},
    {"=",b_equalp},
    {"<",b_lessthanp},
    {"<=",b_lessthaneqp},
    {">",b_greaterthanp},
    {">=",b_greaterthaneqp},
    {"pr-str",b_pr_str},
    {"str",b_str},
    {"prn",b_prn},
    {"println",b_println},
    {"read-string",b_read_string},
    {"slurp",b_slurp},
    {"eval",b_eval},
    {"cons",b_cons},
    {"concat",b_concat},
    {"first",b_first},
    {"second",b_second},
    {"rest",b_rest},
    {"nth",b_nth},
    {"throw",b_throw},
    {"apply",b_apply},
    {"map",b_map},
    {"nil?",b_nil},
    {"true?",b_true},
    {"false?",b_false},
    {"symbol?",b_is_symbol},
    {"symbol",b_symbol},
    {"keyword?",b_is_keyword},
    {"keyword",b_keyword},
    {"vector?",b_is_vector},
    {"vector",b_vector},
    {"hash-map",b_hash_map},
    {"map?",b_is_hash_map},
    {"assoc",b_assoc},
    {"dissoc",b_dissoc},
    {"get",b_get},
    {"contains?",b_contains},
    {"keys",b_keys},
    {"vals",b_vals},
    {"sequential?",b_is_seq},
    {"print-env",b_print_env},
    {"print-mem",b_print_mem},
    {"gc",b_gc},
    {"readline",b_readline},
    {"atom",b_atom},
    {"atom?",b_is_atom},
    {"deref",b_deref},
    {"reset!",b_reset},
    {"swap!",b_swap},
    {"with-meta",b_with_meta},
    {"meta",b_meta},
    {"time-ms",b_time_ms},
    {"string?",b_is_string}
};

static HASH* repl_env = NULL;

/* Insert inbuilt functions into ns namespace and return pointer */
HASH* ns_get(void)
{
    HASH* env;
    VAR* var;
    int i;

    if (repl_env != NULL) return repl_env; /* only one exists */
    env = new_env(101,NULL,NULL,NULL);
    env->closure = true;
    for (i=0;i<(sizeof(core_fn)/sizeof(struct s_builtin));i++) {
        var = new_var();
        var->type = S_BUILTIN;
        var->val.bval = core_fn[i].fn;
        env_put(env,core_fn[i].name,var);
    }
    repl_env = env;
    return env;
}


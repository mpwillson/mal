#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <limits.h>
#include <sys/stat.h>

#include "mal.h"
#include "env.h"
#include "core.h"
#include "printer.h"

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
    var->val.ival = count(list->var->val.lval);
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
    if (islist(list->var->type) && list->var->val.lval == NULL) {
        return &var_true;
    }
    else {
        return &var_false;
    }
}

/* forward declaration */
bool list_equalp(LIST*,LIST*);

VAR* var_equalp(VAR* v1, VAR* v2)
{
    bool eq;

    /* all lists types are equal in mal? */
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
        case S_LIST:
        case S_HASHMAP:
        case S_VECTOR:
        case S_ROOT:
            eq = list_equalp(v1->val.lval,v2->val.lval);
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

VAR* b_equalp(LIST* list)
{
    LIST *l1,*l2;
    VAR* v1= NULL ,*v2 = NULL;

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

static char buffer[1024];

VAR* pr_str(LIST* list,bool readable)
{
    LIST* elt;
    VAR* var = new_var();

    buffer[0] = '\0';
    elt = list;
    while (elt != NULL) {
        strcat(buffer,print_str(elt->var,readable));
        if (readable) strcat(buffer," ");
        elt = elt->next;
    }
    if (readable) buffer[strlen(buffer)-1] = '\0';
    var->type = S_STR;
    var->val.pval = buffer;
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

    buffer[0] = '\0';
    elt = list;
    while (elt != NULL) {
        strcat(buffer,print_str(elt->var,readable));
        strcat(buffer," ");
        elt = elt->next;
    }
    buffer[strlen(buffer)-1] = '\0';
    printf("%s\n",buffer);
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
            error.val.pval = mal_error("unable to open file: '%s'",fn);
            return &error;
        }
        if (fstat(fileno(in),&statbuf) != 0) {
            error.val.pval = mal_error("fstat failed for file '%s'",fn);
            fclose(in);
            return &error;
        }
        /* is this a regular file? */
        if (!S_ISREG(statbuf.st_mode)) {
            error.val.pval = mal_error("file '%s' is not a regular file",fn);
            fclose(in);
            return &error;
        }
        /* acquire memory to hold file contents */
        slurp_buf = (char *) malloc(statbuf.st_size+1);
        if (slurp_buf == NULL) {
            error.val.pval = mal_error("no memory for slurp buffer");
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
                error.val.pval = mal_error("internal error - buffer overflow");
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

VAR* b_cons(LIST* list)
{
    VAR* var;
    LIST* elt;

    if (list ==  NULL) return &var_nil;
    var = list->var;
    elt = list->next;
    if (elt->var->type == S_LIST || elt->var->type == S_VECTOR) {
        return insert(var,elt->var);
    }
    else {
        error.val.pval = mal_error("cons target not a list");
        return &error;
    }
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
    {"cons",b_cons}
};

static ENV* repl_env = NULL;

/* Insert inbuilt functions into ns namespace and return pointer */
ENV* ns_get(void)
{
    ENV* env = new_env(101,NULL,NULL,NULL);
    VAR* var;
    int i;

    if (repl_env != NULL) return repl_env; /* only one exists */
    
    for (i=0;i<(sizeof(core_fn)/sizeof(struct s_builtin));i++) {
        var = new_var();
        var->type = S_BUILTIN;
        var->val.bval = core_fn[i].fn;
        env_put(env,core_fn[i].name,var);
    }
    repl_env = env;
    return env;
}


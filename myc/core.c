#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "mal.h"
#include "env.h"
#include "core.h"

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
            result = list->var;
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
    VAR* var = new_var();

    var->type = S_LIST;
    var->val.lval = list;
    return var;
}

struct s_builtin core_fn[] =
{
    {"+",b_plus},
    {"-",b_minus},
    {"*",b_mul},
    {"/",b_div},
    {"list",b_list},
    {"count",b_count}
};
 
/* Insert inbuilt functions into ns namespace and return pointer */
ENV* ns_get(void)
{
    ENV* env = new_env(101,NULL,NULL,NULL);
    VAR* var;
    int i;

    for (i=0;i<(sizeof(core_fn)/sizeof(struct s_builtin));i++) {
        var = new_var();
        var->type = S_BUILTIN;
        var->val.bval = core_fn[i].fn;
        env_put(env,core_fn[i].name,var);
    }
    return env;
}


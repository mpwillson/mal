#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

#include "mal.h"
#include "reader.h"
#include "printer.h"
#include "env.h"

#define BUFSIZE 1024

static char errmsg[BUFSIZE];

char* mal_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap,fmt);
    vsprintf(errmsg,fmt,ap);
    va_end(ap);
    return errmsg;
}

int length(LIST* list)
{
    int len = 0;
    LIST* elt = list;
    
    while (elt != NULL) {
        len++;
        elt = elt->next;
    }
    return len;
}

VAR* var_plus(VAR* total,VAR* new)
{
    /* VAR* var; */
    total->val.ival += new->val.ival;
    return total;
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
}

VAR* var_mul(VAR* total,VAR* new)
{
    total->val.ival *= new->val.ival;
    return total;
}

VAR* var_div(VAR* total,VAR* new)
{
    total->val.ival = total->val.ival/new->val.ival;
    return total;
}

VAR* var_minus(VAR* total,VAR* new)
{
    total->val.ival -= new->val.ival;
    return total;
}

VAR* arith(FUN fun,VAR* result,LIST* list)
{
    LIST* elt;
    int len;
    
    elt = list;
    result->type = S_INT;
    result->val.ival = 0;
    if (fun == var_mul) {
        result->val.ival = 1;
    }
    else if ((fun == var_div || fun == var_minus) && list != NULL) {
        len = length(list);
        if (len != 1) {
            result = list->var;
            elt = list->next;
        }
    }
    while (elt != NULL) {
        result = fun(result,elt->var);
        elt = elt->next;
    }
    return result;
}

VAR* read(char* s)
{
    VAR *var;
    
    init_lexer(s);
    var = read_list(S_ROOT,')');
    return var;
}

/* forward declare of eval for eval_ast */
VAR* eval(VAR*,ENV*);

/* TDB: Fix memory leaks */

VAR* eval_ast(VAR* ast, ENV* env)
{
    VAR* var, *evaled_var;
    VAR* list_var = new_var();
    LIST* list = NULL;
    LIST* elt;

    if (ast->type == S_VAR) {
        var = env_get(env,ast->val.pval);
        if (var == NULL) {
            list_var->type = S_ERROR;
            list_var->val.pval = mal_error("'%s' not found",ast->val.pval);
            return list_var;
        }
        return var;
    }
    else if (list_type(ast->type) || ast->type == S_ROOT) { 
        elt = ast->val.lval;
        while (elt != NULL) {
            evaled_var = eval(elt->var,env);
            if (evaled_var->type == S_ERROR) {
                return evaled_var;
            }
            else {
                list = append(list,evaled_var);
            }
            elt = elt->next;
        }
        list_var->type = ast->type;
        list_var->val.lval = list;
        return list_var;
    }
    return ast;
}

VAR* eval(VAR* ast,ENV* env)
{
    VAR* eval_list;
    LIST* elt;
    VAR* result = new_var();

    if (ast->type == S_LIST) {
        eval_list = eval_ast(ast,env);
        if (eval_list->type != S_ERROR) {
            elt = eval_list->val.lval;
            if (elt->var->function != NULL) {
                eval_list = arith(elt->var->function,result,elt->next);
            }
            else {
                eval_list->type = S_ERROR;
                eval_list->val.pval =
                    mal_error("%s not callable.",
                              print_str((eval_list->val.lval)->var,true));
            }   
        }
        return eval_list;
    }
    else {
        return eval_ast(ast,env);
    }
}

char* print(VAR* var)
{
    return print_str(var,true);
}
VAR arith_op[] =
{
    {S_VAR,var_plus,"+"},
    {S_VAR,var_minus,"-"},
    {S_VAR,var_mul,"*"},
    {S_VAR,var_div,"/"},
};
        
char* rep(char* s)
{
    ENV* env = new_env(101);
    int i;
    
    for (i=0;i<(sizeof(arith_op)/sizeof(VAR));i++) {
        env_put(env,arith_op[i].val.pval,arith_op+i);
    }
    return print(eval(read(s),env));
}

int main(void)
{
    char buf[BUFSIZE+1];
    char* bufread;
    bool at_eof = false;

    while (!at_eof) {
        fprintf(stdout,"user> ");
        fflush(stdout);
        bufread = fgets(buf,BUFSIZE,stdin);
        at_eof = feof(stdin) || bufread == NULL;
        if (bufread) {
            fprintf(stdout,"%s\n",rep(buf));
        }
    }
    fprintf(stdout,"\n");
    return 0;
}

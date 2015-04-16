#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "mal.h"
#include "reader.h"
#include "printer.h"
#include "env.h"

#define BUFSIZE 1024

static char errmsg[BUFSIZE];

/* For error returns */
static VAR error = {S_ERROR,NULL,NULL};

void mal_die(char* msg)
{
    fprintf(stderr,"mal: FATAL: %s\n",msg);
    exit(1);
}

char* mal_error(const char *fmt, ...)
{
    va_list ap;

    va_start(ap,fmt);
    vsprintf(errmsg,fmt,ap);
    va_end(ap);
    return errmsg;
}

void free_var(VAR* var)
{
    if (isstr(var->type)) {
        free(var->val.pval);
    }
    else if (islist(var->type)) {
        free_list(var->val.lval);
    }
    free(var);
}

void free_list(LIST* list)
{
    LIST* elt, *last_elt;

    elt= list;
    while (elt != NULL) {
        last_elt = elt;
        free_var(elt->var);
        elt = elt->next;
        free(last_elt);
    }
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

FN* new_fn()
{
    FN* fn;

    fn = (FN*) malloc(sizeof(FN));
    if (fn == NULL) {
        mal_die("out of memory at new_fn");
    }
    fn->args = NULL;
    fn->forms = NULL;
    fn->env = NULL;
    return fn;
}

VAR* list_to_var(int type,LIST* list)
{
    VAR* var = new_var();

    var->type = type;
    var->val.lval = list;
    return var;
}

/* Construct function environment from LIST* list.
 * First element is the list of formal arguments.
 * Rrmaining elements are the body of the function, which is executed
 * as a do form.
 * TDB Add error checking.
 */
VAR* make_fn(LIST* list,ENV *env)
{
    FN* fn = new_fn();
    VAR* fn_var = new_var();
 
    fn->args = list->var;
    fn->forms = list->next;
    env->closure = true;
    fn->env = env;
    fn_var->type = S_FN;
    fn_var->val.fval = fn;
    return fn_var;
}

/* forward declare of eval */
VAR* eval(VAR*,ENV*);
VAR* eval_ast(VAR*,ENV*);

VAR* do_form(LIST* form,ENV* env)
{
    VAR* result;
    LIST* elt;
    
    result = &var_nil;
    elt = form;
    while (elt != NULL) {
        result = eval(elt->var,env);
        elt = elt->next;
    }
    return result;
}
    
VAR* execute_fn(VAR* fn, LIST* args)
{
    VAR* exprs = new_var();
    ENV* env;
    VAR* evaled_list;

    exprs->type = S_LIST;
    exprs->val.lval = args;
    /* printf("execute_fn: %s, with args: %s\n",print_str(fn,true), */
    /*        print_str(exprs,true)); */
    env = new_env(37,fn->val.fval->env,fn->val.fval->args,exprs);
    evaled_list = do_form(fn->val.fval->forms,env);
    free(exprs);
    env_free(env);
    return evaled_list;
}

/* TDB: Fix memory leaks */

VAR* eval_ast(VAR* ast, ENV* env)
{
    VAR* var, *evaled_var;
    VAR* list_var = new_var();
    LIST* list = NULL;
    LIST* elt;

    if (ast->type == S_SYM) {
        var = env_get(env,ast->val.pval);
        if (var == NULL) {
            error.val.pval = mal_error("'%s' not found",ast->val.pval);
            return &error;
        }
        return var;
    }
    else if (islist(ast->type)) { 
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
    LIST* elt,*env_elt;
    VAR* result = new_var();
    ENV* new;
    
    if (ast->type == S_LIST) {
        elt = ast->val.lval;
        if (strcmp(elt->var->val.pval,"def!") == 0) {
            elt = elt->next;
            eval_list =  eval(elt->next->var,env);
            env_put(env,elt->var->val.pval,eval_list);
            return eval_list;
        }
        else if (strcmp(elt->var->val.pval,"let*") == 0) {
            new = new_env(101,env,NULL,NULL);
            elt = elt->next;
            if (elt->var->type == S_LIST || elt->var->type == S_VECTOR) {
                env_elt = elt->var->val.lval;
                while (env_elt != NULL) {
                    if (env_elt->next == NULL) {
                        error.val.pval = mal_error("value expected");
                        return &error;
                    }
                    eval_list = eval(env_elt->next->var,new);
                    env_put(new,env_elt->var->val.pval,eval_list);
                    env_elt = env_elt->next->next;
                }
            }
            else {
                error.val.pval = mal_error("list expected");
                return &error;
            }
            if (elt->next != NULL) {
                eval_list = eval(elt->next->var,new);
                env_free(new);
            }
            else { /* nothing to eval */
                return &var_nil;
            }
            return eval_list;
        }
        else if (strcmp(elt->var->val.pval,"do") == 0) {
            return do_form(elt->next,env);
        }
        else if (strcmp(elt->var->val.pval,"if") == 0) {
            elt = elt->next;
            eval_list = &var_nil;
            if (elt != NULL) {
                eval_list = eval(elt->var,env);
                elt = elt->next;
                if (eval_list->type != S_NIL &&
                    eval_list->type != S_FALSE) {
                    if (elt != NULL) {
                        eval_list = eval(elt->var,env);
                    }
                    else {
                        return &var_nil;
                    }
                }
                else {
                    if (elt != NULL) elt = elt->next;
                    if (elt != NULL) eval_list = eval(elt->var,env);
                }
            }
            return eval_list;
        }
        else if (strcmp(elt->var->val.pval,"fn*") == 0) {
            return make_fn(elt->next,env);
        }
        eval_list = eval_ast(ast,env);
        if (eval_list->type != S_ERROR) {
            elt = eval_list->val.lval;
            if (elt->var->function != NULL) {
                eval_list = arith(elt->var->function,result,elt->next);
            }
            else if (elt->var->type == S_FN) {
                eval_list = execute_fn(elt->var,elt->next);
                return eval_list;
            }
            else {
                error.val.pval =
                    mal_error("'%s' not callable",
                              print_str(elt->var,true));
                return &error;
            }
        }
        return eval_list;
    }
    else {
        return eval_ast(ast,env);
    }
}

VAR* repl_read(char* s)
{
    VAR *var;
    
    init_lexer(s);
    var = read_list(S_ROOT,')');
    return var;
}

char* print(VAR* var)
{
    return print_str(var,true);
}
VAR arith_op[] =
{
    {S_SYM,var_plus,"+"},
    {S_SYM,var_minus,"-"},
    {S_SYM,var_mul,"*"},
    {S_SYM,var_div,"/"},
};
        
char* rep(char* s,ENV* env)
{
    char* output;

    output = print(eval(repl_read(s),env));
    return output;
}

int main(void)
{
    char buf[BUFSIZE+1];
    char* bufread;
    bool at_eof = false;
    ENV* env = new_env(101,NULL,NULL,NULL);
    int i;
    VAR* var;
    
    for (i=0;i<(sizeof(arith_op)/sizeof(VAR));i++) {
        var = new_var();
        var->type = S_SYM;
        var->function = arith_op[i].function;
        var->val.pval = strsave(arith_op[i].val.pval);
        env_put(env,strsave(arith_op[i].val.pval),var);
    }
    while (!at_eof) {
        bufread = readline("user> ");
        at_eof = feof(stdin) || bufread == NULL;
        if (bufread) {
            fprintf(stdout,"%s\n",rep(bufread,env));
            if (strlen(bufread) > 0) add_history(bufread);
        }
        free(bufread);
    }
    env_free(env);
    fprintf(stdout,"\n");
    return 0;
}

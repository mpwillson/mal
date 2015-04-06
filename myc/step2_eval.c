#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mal.h"
#include "reader.h"
#include "printer.h"
#include "env.h"

#define BUFSIZE 1024


void mal_error(char *msg)
{
    fprintf(stderr,"mal: %s\n",msg);
    exit(1);
}

VAR* read(char* s)
{
    VAR *var;
    
    init_lexer(s);
    var = read_list(S_LIST,')');
    var->type = S_ROOT;
    return var;
}

VAR* reduce(FUN fun,VAR* seed,LIST* list)
{
    VAR* var;
    LIST* elt;

    elt = list;
    while (elt != NULL) {
        var = elt->var;
        seed = fun(seed,elt->var);
        elt = elt->next;
    }
    return seed;
}

VAR* var_add(VAR* sum,VAR* add)
{
    VAR* var;

    /* TBD: if add value is a float, covert seed to float, if an int */
    var = new_var();
    var->type = S_INT;
    var->val.ival = 0;
    return var;
}

/* forward declare of eval for eval_ast */
VAR* eval(VAR*,ENV*);

VAR* eval_ast(VAR* ast, ENV* env)
{
    VAR* var;
    VAR* list_var = new_var();
    LIST* list = NULL;
    LIST* elt = new_elt();
    
    if (ast->type == S_VAR) {
        var = env_get(env,ast->val.pval);
        if (var == NULL) mal_error("symbol not found in environment");
        return var;
    }
    else if (ast->type == S_LIST) {
        elt = ast->val.lval;
        while (elt != NULL) {
            list = append(list,eval(elt->var,env));
            elt = elt->next;
        }
        list_var->type = S_LIST;
        list_var->val.lval = list;
        return list_var;
    }
    return ast;
}

VAR* eval(VAR* var,ENV* env)
{
    VAR* ast = var;
    VAR* eval_list;
    LIST* elt;

    if (ast->type == S_ROOT) {
        elt = ast->val.lval;
        ast = elt->var;
    }
    if (ast->type == S_LIST) {
        eval_list = eval_ast(ast,env);
        printf("eval: should apply: %s\n",print_str(eval_list,true));
        //eval_list = var_add((eval_list->val.lval)->next);
        return eval_list;
    }
    else {
        return eval_ast(ast,env);
    }
    return var;
}

char* print(VAR* var)
{
    return print_str(var,true);
}

char* rep(char* s)
{
    ENV* env = new_env(101);
    VAR* var = new_var();

    var->type = S_VAR;
    var->val.pval = "+";
    env_put(env,"+",var);
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

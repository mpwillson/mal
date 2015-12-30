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
#include "core.h"
    
#define BUFSIZE 1024

#define DEBUG 0
               
static char errmsg[BUFSIZE];

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

char *strsave(char *s)
{
	char *p;

	p = (char *) malloc(strlen(s)+1);
	if (p != NULL) 	strcpy(p,s);
	return p;
}

LIST* new_elt() {
    LIST* elt;
    
    elt = (LIST *) malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_die("out of memory at new_elt.");
    }
    elt->var = NULL;
    elt->next = NULL;
    return elt;
}

VAR* new_var() {
    VAR* var;
    
    var = (VAR *) malloc(sizeof(VAR));
    if (var == NULL)  {
        mal_die("out of memory at new_var.");
    }
    var->type = S_UNDEF;
    var->val.lval = NULL;
    return var;
}

VAR* list2var(LIST* list)
{
    VAR* var = new_var();

    var->type = S_LIST;
    var->val.lval = list;
    return var;
}

VAR* insert(VAR* var, VAR* list)
{
    LIST* elt = new_elt();

    elt->var = var;
    elt->next = list->val.lval;
    list->val.lval = elt;
    return list;
}

LIST* append(LIST* list,VAR* var)
{
    LIST *elt,*current;

    elt = new_elt();
    elt->var = var;
    if (list == NULL) {
        return elt;
    }
    else {
        current = list;
        while (current->next != NULL) current = current->next;
        current->next = elt;
    }
    return list;
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

/* Construct function environment from LIST* list.
 * First element is the list of formal arguments.
 * Rrmaining elements are the body of the function, which is executed
 * as a do form.
 * TDB Add error checking.
 */
VAR* make_fn(LIST* list,HASH *env)
{
    FN* fn = new_fn();
    VAR* fn_var = new_var();
 
    fn->args = list->var;
    fn->forms = (list->next!=NULL?list->next->var:&var_nil);
    env->closure = true;
    fn->env = env;
    fn_var->type = S_FN;
    fn_var->val.fval = fn;
    return fn_var;
}

/* forward declare of eval */
VAR* eval(VAR*,HASH*);
VAR* eval_ast(VAR*,HASH*);

VAR* do_form(LIST* form,HASH* env)
{
    LIST* elt, *new_list = NULL;
    VAR* result;
    
    if (form == NULL) return &var_nil;
    if (DEBUG) printf("do_form: %s\n",print_str(form->var,true));
    /* slice off last element of form */
    elt = form;
    while (elt->next != NULL) {
        new_list = append(new_list,elt->var);
        elt = elt->next;
    }
    if (new_list != NULL) {
        eval_ast(list2var(new_list),env);
    }
    if (DEBUG) printf("do_form2: %s\n",print_str(elt->var,true));
    return elt->var;
}

/* TDB: Fix memory leaks */

VAR* eval_ast(VAR* ast, HASH* env)
{
    VAR* var, *evaled_var;
    VAR* list_var = new_var();
    LIST* list = NULL;
    LIST* elt;

    if (DEBUG) printf("eval_ast: ast: %s\n",print_str(ast,true));
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

VAR* def_form(LIST* elt,HASH* env)
{
    VAR* evaled;

    if (elt == NULL) return &var_nil;
    evaled = eval(elt->next->var,env);
    env_put(env,elt->var->val.pval,evaled);
    return evaled;
}

HASH* let_env(LIST* elt,HASH *env)
{
    HASH* new;
    VAR* eval_list = &var_nil;
    LIST* env_elt;

    if (elt == NULL) return NULL;
    new = new_env(101,env,NULL,NULL);
    if (elt->var->type == S_LIST || elt->var->type == S_VECTOR) {
        env_elt = elt->var->val.lval;
        while (env_elt != NULL) {
            if (env_elt->next == NULL) {
                free(env);
                return NULL;
            }
            eval_list = eval(env_elt->next->var,new);
            env_put(new,env_elt->var->val.pval,eval_list);
            env_elt = env_elt->next->next;
        }
    }
    return new;
}

VAR* if_form(LIST* elt, HASH* env)
{
    VAR* eval_list;
    
    eval_list = &var_nil;
    if (elt != NULL) {
        eval_list = eval(elt->var,env);
        elt = elt->next;
        if (eval_list->type != S_NIL &&
            eval_list->type != S_FALSE) {
            if (elt != NULL) {
                eval_list = elt->var;
            }
            else {
                return &var_nil;
            }
        }
        else {
            eval_list = &var_nil;
            if (elt != NULL) elt = elt->next;
            if (elt != NULL) eval_list = elt->var;
        }
    }
    return eval_list;
}

VAR* eval(VAR* ast,HASH* env)
{
    VAR* eval_list;
    LIST* elt,*env_elt;
    FN* fn;

    while (true) {
        if (DEBUG) printf("eval: %s\n",print_str(ast,true));
        if (ast->type == S_LIST && ast->val.lval != NULL) {
            elt = ast->val.lval;
            if (strcmp(elt->var->val.pval,"def!") == 0) {
                return def_form(elt->next,env);
            }
            else if (strcmp(elt->var->val.pval,"let*") == 0) {
                env = let_env(elt->next,env);
                if (env == NULL) {
                    error.val.pval = mal_error("malformed binding form");
                    return &error;
                }
                if (elt->next != NULL && elt->next->next != NULL) {
                    ast = elt->next->next->var;
                }
                else {
                    return &var_nil;
                }
            }
            else if (strcmp(elt->var->val.pval,"do") == 0) {
                ast = do_form(elt->next,env);
            }
            else if (strcmp(elt->var->val.pval,"if") == 0) {
                ast = if_form(elt->next,env);
            }
            else if (strcmp(elt->var->val.pval,"fn*") == 0) {
                return make_fn(elt->next,env);
            }
            else {
                eval_list = eval_ast(ast,env);
                if (eval_list->type != S_ERROR) {
                    elt = eval_list->val.lval;
                    if (elt->var->type == S_BUILTIN) {
                        eval_list = elt->var->val.bval(elt->next);
                        return eval_list;
                    }               
                    else if (elt->var->type == S_FN) {
                        fn = elt->var->val.fval;
                        env = new_env(37,fn->env,
                                      fn->args,
                                      list2var(elt->next));
                        ast = fn->forms;
                    }
                    else {
                        error.val.pval = mal_error("'%s' not callable",
                                               print_str(elt->var,true));
                        return &error;
                    }
                }
                else {
                    return eval_list;
                }
            }

        }
        else {
            return eval_ast(ast,env);
        }
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
     
char* rep(char* s,HASH* env)
{
    char* output;

    output = print(eval(repl_read(s),env));
    return output;
}

int execute_program(char* filename,int nargs,char* argv[],HASH* env)
{
    char cmd[BUFSIZE+1];
    LIST* list = NULL;
    VAR* var;

    sprintf(cmd,"(load-file \"%s\")",filename);
    while (nargs > 0) {
        var = new_var();
        var->type = S_STR;
        var->val.pval = *argv++;
        list = append(list,var);
        nargs--;
    }
    env_put(env,"*ARGV*",list2var(list));
    rep(cmd,env);
    return 0;
}
            
int main(int argc, char* argv[])
{
    char* bufread;
    bool at_eof = false;
    HASH* env = ns_get();

    /* define mal functions */
    rep("(def! not (fn* [x] (if x false true)))",env);
    rep("(def! load-file (fn* (f) (eval (read-string "
        "(str \"(do\" (slurp f) \"\\n)\")))))",env);

    env_put(env,"*ARGV*",list2var(NULL));
    
    /* execute mal program, if found */
    if (argc > 1) {
        execute_program(argv[1],(argc-2),argv+2,env);
    }
    else {
        while (!at_eof) {
            bufread = readline("user> ");
            at_eof = feof(stdin) || bufread == NULL;
            if (bufread) {
                if (strlen(bufread) > 0){
                    add_history(bufread);
                    fprintf(stdout,"%s\n",rep(bufread,env));
                }
            }
            free(bufread);
        }
        env_free(env);
        fprintf(stdout,"\n");
    }
    return EXIT_SUCCESS;
}

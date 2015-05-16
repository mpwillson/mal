#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <setjmp.h>

#include "mal.h"
#include "reader.h"
#include "printer.h"
#include "env.h"
#include "core.h"
    
#define BUFSIZE 1024

#define DEBUG 0

/* Define atoms */
VAR quote = {
    S_SYM,{"quote"}
};
VAR quasiquote = {
    S_SYM,{"quasiquote"}
};
VAR unquote = {
    S_SYM,{"unquote"}
};
VAR splice = {
    S_SYM,{"splice-unquote"}
};
VAR deref = {
    S_SYM,{"deref"}
};
VAR meta = {
    S_SYM,{"with-meta"}
};
VAR var_nil = {
    S_NIL,{NULL}
};
VAR var_true = {
    S_TRUE,{NULL}
};
VAR var_false = {
    S_FALSE,{NULL}
};
VAR empty_list = {
    S_LIST,{NULL}
};

VAR do_var = {S_SYM,{"do"}};

/* For error returns */
VAR* thrown_var;
VAR error = {S_ERROR,{NULL}};
jmp_buf jmp_env;

void mal_die(char* msg)
{
    fprintf(stderr,"mal: FATAL: %s\n",msg);
    exit(1);
}

VAR* mal_error(const char *fmt, ...)
{
    va_list ap;
    static char errmsg[BUFSIZE];

    va_start(ap,fmt);
    vsprintf(errmsg,fmt,ap);
    va_end(ap);
    error.val.pval = errmsg;
    return &error;
}

void throw(VAR* thrown)
{
    thrown_var = thrown;
    longjmp(jmp_env,1);
    return;
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

    elt = list;
    while (elt != NULL) {
        last_elt = elt;
        free_var(elt->var);
        elt = elt->next;
        free(last_elt);
    }
}

VAR* but_last(LIST* list)
{
    LIST* elt, *new_list = NULL;

    if (!list) return &empty_list;
    elt = list;
    while (elt->next != NULL) {
        new_list = append(new_list,elt->var);
        elt = elt->next;
    }
    return list2var(new_list);
}

VAR* last(LIST* list)
{
    LIST* elt;

    if (!list) return &var_nil;
    elt = list;
    while (elt->next != NULL) {
        elt = elt->next;
    }
    return elt->var;
}    

VAR* list2var(LIST* list)
{
    VAR* var = new_var();

    var->type = S_LIST;
    var->val.lval = list;
    return var;
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

bool is_pair(VAR* var)
{
    return (islist(var->type) && var->val.lval);
}

VAR* first(VAR* var)
{
    if (is_pair(var))
        return var->val.lval->var;
    return NULL;
}

VAR* second(VAR* var)
{
    if (is_pair(var) && var->val.lval->next)
        return var->val.lval->next->var;
    return NULL;
}

VAR* rest(VAR* var)
{
    VAR* rest_var = NULL;

    if (is_pair(var) && var->val.lval->next) {
        rest_var = new_var();
        rest_var->type = S_LIST;
        rest_var->val.lval = var->val.lval->next;
    }
    return rest_var;
}

VAR* try_catch_form(LIST* list, ENV* env)
{
    VAR *try_form, *catch_form, *catch_var, *v, *ast;
    jmp_buf jmp_env_save;

    try_form = cons(&do_var,(but_last(list))->val.lval);
    catch_form = last(list);
    if (catch_form && (v=first(catch_form)) && v->type == S_SYM &&
        strcmp(v->val.pval,"catch*") == 0) {
        memcpy(jmp_env_save,jmp_env,sizeof(jmp_env));
        catch_var = cons(second(catch_form),NULL);
        catch_form = cons(&do_var,(rest(rest(catch_form)))->val.lval);
        if (setjmp(jmp_env) == 0) {
            ast = eval(try_form,env);
        }
        else {
            ast = eval(catch_form,new_env(37,env,catch_var,
                                          cons(thrown_var,NULL)));
        }
        memcpy(jmp_env,jmp_env_save,sizeof(jmp_env));
    }
    else {
        throw(mal_error("try with no catch"));
        return &error;
    }
    return ast;
}

/* Construct function environment from LIST* list.
 * First element is the list of formal arguments.
 * Remaining elements are the body of the function, wrapped in a do
 * form.
 */
VAR* fn_form(LIST* list,ENV *env,int type)
{
    FN* fn = new_fn();
    VAR* fn_var = new_var();

    if (list) {
        fn->args = list->var;
        fn->forms = cons(&do_var,list->next);
        env->closure = true;
        fn->env = env;
        fn_var->type = type;
        fn_var->val.fval = fn;
        return fn_var;
    }
    return &var_nil;
}

FN* is_macro_call(VAR* ast,ENV* env)
{
    VAR* var;
    
    var = first(ast);
    if (var && var->type == S_SYM) {
        var = env_get(env,var->val.pval);
        if (var && var->type == S_MACRO) return var->val.fval;
    }
    return NULL;
}

/* Set MAL_MACRO to 0 to provide alternate (more normal) macro
 * definition, one that supports explicit macro arguments. Note that
 * in-built macro definitions will not evaluate with the alternate
 * setting. */
#define MAL_MACRO 1

#if MAL_MACRO == 1
VAR* defmacro_form(LIST* list,ENV *env)
{
    VAR* macro_var = &var_nil, *macro_body;
    VAR* name;

    if (list) {
        name = list->var;
        if (list->next) {
            macro_body = rest(list->next->var);
            macro_var = fn_form(macro_body->val.lval,env,S_MACRO);
            env_put(env,name->val.pval,macro_var);
        }
    }
    return macro_var;
}

#else
/* Macros with arguments (defmacro! m [x y] (forms)) */
VAR* defmacro_form(LIST* list,ENV *env)
{
    VAR* macro_var = &var_nil;
    VAR* name;

    if (list) {
        name = list->var;
        if (list->next) {
            macro_var = fn_form(list->next,env,S_MACRO);
            env_put(env,name->val.pval,macro_var);
        }       
    }
    return macro_var;
}
#endif

VAR* macroexpand(VAR* ast, ENV* env)
{
    FN* macro;

    while ((macro = is_macro_call(ast,env))) {
        ast = eval(macro->forms,new_env(37,macro->env,macro->args,
                              list2var(ast->val.lval->next)));
    }   
    return ast;
}
      
VAR* do_form(LIST* form,ENV* env)
{
    VAR* var;
    /* LIST* new_list = NULL; */
    
    if (form == NULL) return &var_nil;
    if (DEBUG) printf("do_form: %s\n",print_str(form->var,true,true));
    eval_ast(but_last(form),env);
    var = last(form);
    if (DEBUG) printf("do_form2: %s\n",print_str(var,true,true));
    return var;
}

VAR* def_form(LIST* elt,ENV* env)
{
    VAR* evaled;

    if (elt == NULL) return &var_nil;
    evaled = eval(elt->next->var,env);
    env_put(env,elt->var->val.pval,evaled);
    return evaled;
}

ENV* let_env(LIST* elt,ENV *env)
{
    ENV* new;
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

VAR* if_form(LIST* elt, ENV* env)
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

VAR* handle_quasiquote(VAR* ast)
{
    LIST *elt, *new_list = NULL;
    VAR *var, *operator, *rest;
    static VAR cons = {S_SYM,{"cons"}};
    static VAR concat = {S_SYM,{"concat"}};

    if (DEBUG) printf("qq entry: %s\n",print_str(ast,true,true));
    if (!is_pair(ast)) {
        return list2var(append(append(NULL,&quote),ast));
    }
    if ((first(ast)) == &unquote ) {
        return second(ast);
    }
    if (first(first(ast)) == &splice){
        var = second(first(ast));
        if (!var) var = &var_nil;
        if (DEBUG) printf("splice: %s\n",print_str(var,true,true));
        operator = &concat;
    } else {
        var = handle_quasiquote(first(ast));
        operator = &cons;
    }
    elt = ast->val.lval->next; /* remainder of ast */
    if (DEBUG && elt) printf("qq: recursive arg: %s\n",
                             print_str(list2var(elt),true,true));
    if (elt) {
        new_list = append(new_list,handle_quasiquote(list2var(elt)));
        rest = new_list->var;
    }
    else {
        rest = list2var(NULL);
    }
    new_list = append(append(append(NULL,operator),var),rest);
    var = list2var(new_list);
    if (DEBUG) printf("qq: exit %s\n",print_str(var,true,true));
    return var;
}

VAR* eval_ast(VAR* ast, ENV* env)
{
    VAR* var, *evaled_var;
    VAR* list_var = new_var();
    LIST* list = NULL;
    LIST* elt;

    if (DEBUG) printf("eval_ast: ast: %s\n",print_str(ast,true,true));
    if (ast->type == S_SYM) {
        var = env_get(env,ast->val.pval);
        if (var == NULL) {
            // error.val.pval =
            throw(mal_error("'%s' not found",ast->val.pval));
            return &error;
        }
        return var;
    }
    else if (islist(ast->type)) { 
        elt = ast->val.lval;
        while (elt != NULL) {
            evaled_var = eval(elt->var,env);
            if (evaled_var->type == S_ERROR) {
                throw(mal_error(evaled_var->val.pval));
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
    FN* fn;

    while (true) {
        if (DEBUG) printf("eval: %s\n",print_str(ast,true,true));
        if (ast->type == S_LIST && ast->val.lval != NULL) {
            ast = macroexpand(ast,env);
            if (ast->type != S_LIST) return ast;
            elt = ast->val.lval;
            if (elt->var->type == S_SYM) {
                if (strcmp(elt->var->val.pval,"def!") == 0) {
                    return def_form(elt->next,env);
                }
                else if (strcmp(elt->var->val.pval,"let*") == 0) {
                    env = let_env(elt->next,env);
                    if (env == NULL) {
                        throw(mal_error("malformed binding form"));
                        return &error;
                    }
                    if (elt->next != NULL && elt->next->next != NULL) {
                        ast = elt->next->next->var;
                        continue;
                    }
                    else {
                        return &var_nil;
                    }
                }
                else if (strcmp(elt->var->val.pval,"do") == 0) {
                    ast = do_form(elt->next,env); continue;
                }
                else if (strcmp(elt->var->val.pval,"if") == 0) {
                    ast = if_form(elt->next,env); continue;
                }
                else if (strcmp(elt->var->val.pval,"fn*") == 0) {
                    return fn_form(elt->next,env,S_FN);
                }
                else if (strcmp(elt->var->val.pval,"quote") == 0) {
                    return (elt->next?elt->next->var:&var_nil);
                }
                else if (strcmp(elt->var->val.pval,"quasiquote") == 0) {
                    ast = (elt->next?handle_quasiquote(elt->next->var):
                           &var_nil);
                    continue;
                }
                else if (strcmp(elt->var->val.pval,"defmacro!") == 0) {
                    return defmacro_form(elt->next,env);
                }
                else if (strcmp(elt->var->val.pval,"macroexpand") == 0) {
                    return macroexpand(elt->next->var,env);
                }
                else if (strcmp(elt->var->val.pval,"try*") == 0) {
                    return try_catch_form(elt->next,env);
                }
            }
            eval_list = eval_ast(ast,env);
            if (eval_list->type == S_LIST) {
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
                    throw(mal_error("'%s' not callable",
                                    print_str(elt->var,true,true)));
                    return &error;
                }
            }
            else {
                return eval_list;
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
    return print_str(var,true,true);
}
     
char* rep(char* s,ENV* env)
{
    char* output;

    output = print(eval(repl_read(s),env));
    return output;
}

int execute_program(char* filename,int nargs,char* argv[],ENV* env)
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
    if (setjmp(jmp_env) == 0) {
        rep(cmd,env);
    }
    else {
        fprintf(stderr,"%s\n",print_str(&error,true,true));
    }
    return 0;
}
            
int main(int argc, char* argv[])
{
    char* bufread;
    bool at_eof = false;
    ENV* env = ns_get();

    /* define mal functions and macros */
    rep("(def! not (fn* [x] (if x false true)))",env);
    rep("(def! load-file (fn* (f) (eval (read-string "
        "(str \"(do\" (slurp f) \"\\n)\")))))",env);
    rep("(defmacro! or (fn* (& xs) "
        "(if (empty? xs) nil (if (= 1 (count xs)) (first xs) "
        "`(let* (or_FIXME ~(first xs)) (if or_FIXME or_FIXME "
        "(or ~@(rest xs))))))))",env);
    rep("(defmacro! and (fn* (& xs) "
        "(if (empty? xs) true (if (= 1 (count xs)) (first xs) "
        "`(let* (and_FIXME ~(first xs)) (if (not and_FIXME) and_FIXME "
        "(and ~@(rest xs))))))))",env);
    rep("(defmacro! cond (fn* (& xs) "
        "(if (> (count xs) 0) (list 'if (first xs) (if (> (count xs) 1) "
        "(nth xs 1) (throw \"odd number of forms to cond\")) "
        "(cons 'cond (rest (rest xs)))))))",env);
    rep("(defmacro! -> (fn* [x & xs]"
        "(if (empty? xs) x"
        "(let* (x_ (first xs) nelt_ (count x_))"
        "(if (= nelt_ 0) "
        "`(-> (~x_ ~x) ~@(rest xs))"
        "(if (= nelt_ 1)"
        "`(-> (~(first x_) ~x) ~@(rest xs))"
        "`(-> (~(first x_) ~x ~@(rest x_)) ~@(rest xs))))))))",env);
    
    env_put(env,"*ARGV*",list2var(NULL));
    
    /* execute mal program, if found */
    if (argc > 1) {
        execute_program(argv[1],(argc-2),argv+2,env);
    }
    else {
        if (setjmp(jmp_env) != 0) {
            fprintf(stderr,"%s\n",print_str(thrown_var,true,true));
            if (bufread) free(bufread);
        }
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

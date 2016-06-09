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
#include "mem.h"
    
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
VAR do_sym          = {S_SYM,{"do"}};
VAR def             = {S_SYM,{"def!"}};
VAR let             = {S_SYM,{"let*"}};
VAR if_sym          = {S_SYM,{"if"}};
VAR fn_sym          = {S_SYM,{"fn*"}};
VAR defmacro        = {S_SYM,{"defmacro!"}};
VAR macroexpand_sym = {S_SYM,{"macroexpand"}};
VAR try             = {S_SYM,{"try*"}};
VAR catch           = {S_SYM,{"catch*"}};

VAR host_lang = {S_STR,{"myc"}};


/* For error returns */
VAR* thrown_var;
VAR error = {S_ERROR,{NULL}};
jmp_buf jmp_env;

void mal_die(char* msg)
{
    fprintf(stderr,"mal: FATAL: %s\n",msg);
    exit(EXIT_FAILURE);
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


VEC* mkvector(LIST* list)
{
    VEC* vec;
    VAR** v;
    LIST* elt;
    int size;

    size = count(list);
    vec = new_vec(size);
    elt = list;
    v = vec->vector;
    while (elt) {
        *v++ = elt->var;
        elt = elt->next;
    }
    return vec;
}

HASH* mkhashmap(LIST* list)
{
    HASH* hash;
    LIST* elt;
    int size;

    size = count(list);
    if (size%2 == 1) return NULL;
    hash = new_hash(37);
    elt = list;
    while (elt) {
        env_put(hash,print_str(elt->var,false,true),elt->next->var);
        elt = elt->next->next;
    }
    return hash;
}

/* Return list-like object (vector, hash) as a list (sequence) */
VAR* seq(VAR* var)
{
    LIST* list = NULL;
    VEC* vec;
    VAR* seq_var;
    HASH* hash;
    ITER* iter;
    SYM* sp;
    int i;
    char ch_str[2] = " ", *s;

    if (var->type == S_VECTOR) {
        vec = var->val.vval;
        for (i=0;i<vec->size;i++) {
            list = append(list,vec->vector[i]);
        }
        return (list==NULL?&var_nil:list2var(list));
    }
    else if (var->type == S_HASHMAP) {
        hash = var->val.hval;
        iter = env_iter_init(hash);
        while ((sp = env_next(iter)) != NULL) {
            seq_var = str2var(sp->name);
            list = append(append(list,seq_var),sp->value);
        }
        free(iter);
        return (list==NULL?&var_nil:list2var(list));
    }
    else if (var->type == S_STR) {
        for (s=var->val.pval;*s!='\0';s++) {
            ch_str[0] = *s;
            seq_var = new_var();
            seq_var->type = S_STR;
            seq_var->val.pval = strsave(ch_str);
            list = append(list,seq_var);
        }
        return (list==NULL?&var_nil:list2var(list));
    }
    else if (islist(var->type)) {
        return (var->val.lval==NULL?&var_nil:var);
    }
    return &var_nil;
}

VAR* but_last(LIST* list)
{
    LIST* elt, *new_list = NULL;

    if (!list) return &empty_list;
    if (DEBUG) printf("but_last args: %s\n",print_str(list2var(list),true,true));
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
    if (DEBUG) printf("last args: %s\n",print_str(list2var(list),true,true));
    elt = list;
    while (elt->next != NULL) {
        elt = elt->next;
    }
    if (DEBUG) printf("last return var: %s\n",print_str(elt->var,true,true));
    return elt->var;
}    

VAR* str2var(char* s)
{
    VAR* var = new_var();
    
    var->type = (s[0] == ':'?S_KEYWORD:S_STR);
    var->val.pval = strsave((var->type==S_STR?s:s+1));
    return var;
}

VAR* list2var(LIST* list)
{
    VAR* var = new_var();

    var->type = S_LIST;
    var->val.lval = ref_elt(list); 
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
        current->next = ref_elt(elt);
    }
    return list;
}

bool is_pair(VAR* var)
{
    if (var->type == S_VECTOR )
        return var->val.vval->size>0;
    else {
        return (islist(var->type) && var->val.lval);
    }
}

VAR* first(VAR* var)
{
    if (is_pair(var)) {
        if (var->type == S_VECTOR) {
            return var->val.vval->vector[0];
        }
        else {
            return var->val.lval->var;
        }
    }
    return NULL;
}

VAR* second(VAR* var)
{
    if (is_pair(var)) {
        if (var->type == S_VECTOR) {
            return (var->val.vval->size>1?var->val.vval->vector[0]:&var_nil);
        }
        else if (var->val.lval->next) {            
            return var->val.lval->next->var;
        }
    }
    return NULL;
}

VAR* rest(VAR* var)
{
    VAR* rest_var = NULL;

    if (is_pair(var)) {
        var = seq(var);
        if (var->val.lval->next) {
            rest_var = list2var(var->val.lval->next);
        }
    }
    return rest_var;
}

VAR* try_catch_form(LIST* list, HASH* env)
{
    VAR *try_form, *catch_form, *catch_var, *v, *ast = &var_nil;
    jmp_buf jmp_env_save;

    try_form = cons(&do_sym,(but_last(list))->val.lval);
    catch_form = last(list);
    if (catch_form && (v=first(catch_form)) && v->type == S_SYM &&
        v == &catch) {
        memcpy(jmp_env_save,jmp_env,sizeof(jmp_env));
        catch_var = cons(second(catch_form),NULL);
        catch_form = cons(&do_sym,(rest(rest(catch_form)))->val.lval);
        if (setjmp(jmp_env) == 0) {
            ast = eval(try_form,env);
        }
        else {
            if (setjmp(jmp_env) == 0) {
                ast = eval(catch_form,new_env(37,env,catch_var,
                                          cons(thrown_var,NULL)));
            }
            else {
                memcpy(jmp_env,jmp_env_save,sizeof(jmp_env));
                throw(mal_error("catch* form: %s",
                                print_str(thrown_var,true,true)));
            }
        }
        memcpy(jmp_env,jmp_env_save,sizeof(jmp_env));
    }
    else {
        throw(mal_error("try with no catch"));
    }
    return ast;
}

/* Construct function environment from LIST* list.
 * First element is the list of formal arguments.
 * Remaining elements are the body of the function, wrapped in a do
 * form.
 */
VAR* fn_form(LIST* list,HASH *env,int type)
{
    FN* fn;
    VAR* fn_var = &var_nil;

    if (list) {
        fn = new_fn();
        fn_var = new_var();
        fn->args = list->var;
        fn->forms = cons(&do_sym,ref_elt(list->next));
        if (DEBUG) printf("fn forms: %s\n",print_str(fn->forms,true,true));
        env->closure = true;
        fn->env = env;
        fn_var->type = type;
        fn_var->val.fval = fn;
    }
    return fn_var;
}

FN* is_macro_call(VAR* ast,HASH* env)
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
VAR* defmacro_form(LIST* list,HASH *env)
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
VAR* defmacro_form(LIST* list,HASH *env)
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

VAR* macroexpand(VAR* ast, HASH* env)
{
    FN* macro;

    while ((macro = is_macro_call(ast,env))) {
        ast = eval(macro->forms,new_env(37,macro->env,macro->args,
                              list2var(ast->val.lval->next)));
    }   
    return ast;
}
      
VAR* do_form(LIST* form,HASH* env)
{
    VAR* var;
    
    if (form == NULL) return &var_nil;
    if (DEBUG) printf("do_form: %s\n",print_str(form->var,true,true));
    var = last(form);
    add_active(var);
    eval_ast(but_last(form),env);
    if (DEBUG) printf("do_form2: %s\n",print_str(var,true,true));
    del_active(1);
    return var;
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
        env_elt = seq(elt->var)->val.lval;
        while (env_elt != NULL) {
            if (env_elt->next == NULL) {
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
    if (first(ast) == &unquote ) {
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
    elt = seq(ast)->val.lval->next; /* remainder of ast */
    if (DEBUG && elt) printf("qq: recursive arg: %s\n",
                             print_str(list2var(elt),true,true));
    if (elt) {
        rest = handle_quasiquote(list2var(elt));
    }
    else {
        rest = list2var(NULL);
    }
    new_list = append(append(append(NULL,operator),var),rest);
    var = list2var(new_list);
    if (DEBUG) printf("qq: exit %s\n",print_str(var,true,true));
    return var;
}

VAR* eval_ast(VAR* ast, HASH* env)
{
    VAR* var, *evaled_var;
    VAR* list_var;
    LIST* list = NULL;
    LIST* elt;
    VEC* vec, *vec_copy;
    HASH* hash, *hash_copy;
    ITER* iter;
    SYM* sp;
    int i;

    if (DEBUG) printf("eval_ast: ast: %s\n",print_str(ast,true,true));
    if (ast->type == S_SYM) {
        var = env_get(env,ast->val.pval);
        if (!var) {
            throw(mal_error("'%s' not found",print_str(ast,true,true)));
        }
        return var;
    }
    else if (ast->type == S_VECTOR) {
        vec = ast->val.vval;
        vec_copy = new_vec(vec->size);
        for (i=0;i<vec->size;i++) {
            vec_copy->vector[i] = eval(vec->vector[i],env);
        }
        evaled_var = new_var();
        evaled_var->type = S_VECTOR;
        evaled_var->val.vval = vec_copy;
        return evaled_var;
    }
    else if (ast->type == S_HASHMAP) {
        hash = ast->val.hval;
        hash_copy = new_hash(hash->size);
        iter = env_iter_init(hash);
        while ((sp = env_next(iter)) != NULL) {
            env_put(hash_copy,sp->name,eval(sp->value,env));
        }
        free(iter);
        evaled_var = new_var();
        evaled_var->type = S_HASHMAP;
        evaled_var->val.hval = hash_copy;
        return evaled_var;
    }
    else if (islist(ast->type)) {
        add_active(ast);
        elt = ast->val.lval;
        while (elt != NULL) {
            evaled_var = eval(elt->var,env);
            list = append(list,evaled_var);
            elt = elt->next;
        }
        list_var = new_var();
        list_var->type = ast->type;
        list_var->val.lval = ref_elt(list);
        del_active(1);
        return list_var;
    }
    return ast;
}
    
VAR* eval(VAR* ast,HASH* env)
{
    VAR* eval_list;
    LIST* elt;
    FN* fn = NULL;

    while (true) {
        if (DEBUG) printf("eval: %s\n",print_str(ast,true,true));
        if (ast->type == S_LIST && ast->val.lval != NULL) {
            ast = macroexpand(ast,env);
            if (ast->type != S_LIST) return eval_ast(ast,env);
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
                else if (elt->var == &do_sym) {
                    ast = do_form(elt->next,env); continue;
                }
                else if (elt->var == &if_sym) {
                    ast = if_form(elt->next,env); continue;
                }
                else if (elt->var == &fn_sym) {
                    return fn_form(elt->next,env,S_FN);
                }
                else if (elt->var == &quote) {
                    return (elt->next?elt->next->var:&var_nil);
                }
                else if (elt->var == &quasiquote) {
                    ast = (elt->next?handle_quasiquote(elt->next->var):
                           &var_nil);
                    continue;
                }
                else if (elt->var == &defmacro) {
                    return defmacro_form(elt->next,env);
                }
                else if (elt->var == &macroexpand_sym) {
                    return macroexpand(elt->next->var,env);
                }
                else if (elt->var == &try) {
                    return try_catch_form(elt->next,env);
                }
            }
            eval_list = eval_ast(ast,env);
            /* add_active(ast); NOT REQUIRED? (see others below)*/
            if (eval_list->type == S_LIST) {
                elt = eval_list->val.lval;
                if (elt->var->type == S_BUILTIN) {
                    eval_list = elt->var->val.bval(elt->next);
                    if (fn) env->closure = false;
                    /* del_active(1); */
                    return eval_list;
                }               
                else if (elt->var->type == S_FN) {
                    fn = elt->var->val.fval;
                    env = new_env(37,fn->env,
                                  fn->args,
                                  list2var(elt->next));
                    env->closure = true;
                    ast = fn->forms;
                }
                else {
                    throw(mal_error("'%s' not found",
                                    print_str(elt->var,true,true)));
                    if (fn) env->closure = false;
                    /* del_active(1); */
                    return &error;
                }
            }
            else {
                if (fn) env->closure = false;
                return eval_list;
            }   
            /* del_active(1); */
        }
        else {
            if (fn) env->closure = false;
            return eval_ast(ast,env);
        }
    }
}

VAR* repl_read(char* s)
{
    VAR *var;
    
    init_lexer(s);
    var = read_form(lexer()); /* no need for read_list(S_ROOT,')'); */
    return var;
}

char* print(VAR* var)
{
    return print_str(var,true,true);
}
     
char* rep(char* s,HASH* env)
{
    char* output;
    VAR* current_form,*result;
    
    current_form = repl_read(s);
    env_put(env,"*0",current_form); /* protect from gc */
    result = eval(current_form,env);
    env_put(env,"*1",result);    
    output = print(result);
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
    if (setjmp(jmp_env) == 0) {
        rep(cmd,env);
    }
    else {
        fprintf(stderr,"%s\n",print_str(&error,false,true));
    }
    return 0;
}
            
int main(int argc, char* argv[])
{
    char* bufread;
    bool at_eof = false;
    HASH* env = ns_get();

    /* define mal functions and macros */
    rep("(def! not (fn* [x] (if x false true)))",env);
    rep("(def! load-file (fn* (f) (eval (read-string "
        "(str \"(do\" (slurp f) \"\\n)\")))))",env);
    rep("(def! *gensym-counter* (atom 0))",env);
    rep("(def! gensym "
        "(fn* [] (symbol (str \"G__\" "
        "(swap! *gensym-counter* (fn* [x] (+ 1 x)))))))",env);
    rep("(defmacro! or (fn* (& xs) "
        "(if (empty? xs) nil (if (= 1 (count xs)) (first xs) "
        "(let* (condvar (gensym)) `(let* (~condvar ~(first xs)) "
        "(if ~condvar ~condvar (or ~@(rest xs)))))))))",env);
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
    env_put(env,"*host-language*",&host_lang);
    env_put(env,"*ARGV*",list2var(NULL));
    
    /* execute mal program, if found */
    if (argc > 1) {
        if (setjmp(jmp_env) != 0) {
            fprintf(stderr,"%s\n",print_str(thrown_var,false,true));
            return EXIT_FAILURE;
        }
        else {
            execute_program(argv[1],(argc-2),argv+2,env);
        }
    }
    else {
        rep("(println (str \"Mal [\" *host-language* \"]\"))",env);
        if (setjmp(jmp_env) != 0) {
            fprintf(stderr,"%s\n",print_str(thrown_var,false,true));
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
                free(bufread);
            }
        }
        fprintf(stdout,"\n");
    }
    return EXIT_SUCCESS;
}

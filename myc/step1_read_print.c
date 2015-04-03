#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "reader.h"
#include "symtab.h"

#define BUFSIZE 1024

void mal_error(char *msg)
{
    fprintf(stderr,"mal: %s\n",msg);
    exit(1);
}

/* TBD these two routines should be in symtab.c */

LIST* new_list() {
    LIST* elt;
    
    elt = (LIST *) malloc(sizeof(LIST));
    if (elt == NULL) {
        mal_error("out of memory at new_list.");
    }
    elt->var = NULL;
    elt->next = NULL;
    return elt;
}

VAR* new_var() {
    VAR* var;
    
    var = (VAR *) malloc(sizeof(VAR));
    if (var == NULL)  {
        mal_error("out of memory at new_var.");
    }
    var->type = S_UNDEF;
    var->val.lval = NULL;
    return var;
}

LIST* append(LIST* list,VAR* var)
{
    LIST *elt,*current;

    elt = new_list();
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

VAR* read_list(void)
{
    LIST* form;
    VAR* var;
    int token_type;

    var = new_var();
    form = NULL;
    token_type = lexer();
    while (token_type != S_EOE && token_type != ')') {
        switch (token_type) {
            case '(':
                form = append(form,read_list());
                break;
            default:
                form = append(form,symbolise(token_type,lextok));
        }
        token_type = lexer();
    }
    var->type = S_LIST;
    var->val.lval = form;
    return var;
}

LIST* read(char* s)
{
    VAR *var;
    LIST *root = new_list();
    
    init_lexer(s);
    var = read_list();
    root->var = var;
    return root;
}

LIST* eval(LIST* form)
{
    return form;
}

char* print(LIST* form)
{
    LIST* elt;
    char tok[LEXTOKSIZ+1];
    char *buffer = (char *) malloc(BUFSIZE+1);

    buffer[0] = '\0';
    tok[0] = '\0';
    elt = form;
    while (elt != NULL) {
        switch (elt->var->type) {
            case S_LIST:
                sprintf(tok,"(%s)",print(elt->var->val.lval));
                break;
            case S_INT:
                sprintf(tok,"%d ",elt->var->val.ival);
                break;
            case S_STR:
                sprintf(tok,"\"%s\" ",elt->var->val.pval);
                break;
            case S_VAR:
                sprintf(tok,"%s ",elt->var->val.pval);
                break;
            default:
                mal_error("print: form contains bad type.");
        }
        strcat(buffer,tok);
        elt = elt->next;
    }
    return buffer;
}


char* rep(char* s)
{
    return print(eval(read(s)));
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

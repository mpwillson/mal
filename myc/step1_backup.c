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

LIST* read(char* s)
{
    VAR *var;
    LIST *root = new_list();
    
    init_lexer(s);
    var = read_list();
    var->type = S_ROOT;
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
            case S_ROOT:
                sprintf(tok,"%s",print(elt->var->val.lval));
                break;
            case S_LIST:
                sprintf(tok,"(%s)",print(elt->var->val.lval));
                break;
            case S_INT:
                sprintf(tok,"%d",elt->var->val.ival);
                break;
            case S_FLOAT:
                sprintf(tok,"%f",elt->var->val.fval);
                break;
            case S_STR:
                sprintf(tok,"\"%s\"",elt->var->val.pval);
                break;
            case S_VAR:
                sprintf(tok,"%s",elt->var->val.pval);
                break;
            case S_KEYWORD:
                sprintf(tok,":%s",elt->var->val.pval);
                break;
            default:
                mal_error("print: form contains bad type.");
        }
        if (buffer[0] != '\0') strcat(buffer," ");
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

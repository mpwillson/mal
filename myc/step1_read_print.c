#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "reader.h"
#include "printer.h"

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

VAR* eval(VAR* var)
{
    return var;
}

char* print(VAR* var)
{
    return print_str(var,true);
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

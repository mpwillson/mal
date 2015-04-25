#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char* read(char* s)
{
    return s;
}

char* eval(char *s)
{
    return s;
}

char* print(char* s)
{
    return s;
}

char* rep(char* s)
{
    return print(eval(read(s)));
}

#define BUFSIZE 256

int main(void)
{
    char buf[BUFSIZE+1];
    char* bufread;
    bool at_eof = false;

    while (!at_eof) {
        fprintf(stdout,"lisp: ");
        fflush(stdout);
        bufread = fgets(buf,BUFSIZE,stdin);
        at_eof = feof(stdin) || bufread == NULL;
        if (bufread) {
            fprintf(stdout,"%s",rep(buf));
        }
    }
    fprintf(stdout,"\n");
    
    return 0;
}

    
    

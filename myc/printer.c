/*
 * 	NAME
 *      printer - printer functions for mal
 *
 * 	SYNOPSIS
 *
 *
 * 	DESCRIPTION
 *
 *
 * 	NOTES
 *
 *
 * 	MODIFICATION HISTORY
 * 	Mnemonic	Rel	Date	Who
 *
 * 		Written.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "mal.h"
#include "reader.h"
#include "printer.h"

#define BUFSIZE 1024

static char buf[BUFSIZE];

char *stringify (char *s)
{
    char *p;

    p = buf;
    while (*s != '\0') {
        if (*s == '"') {
            *p++ = '\\';
        }
        else if (*s == '\n') {
            *p++ = '\\';
            *p++ = 'n';
            *s = *s + 1;
            continue;
        }
        *p++ = *s++;
    }
    *p = '\0';
    return buf;
}

char* print_str(VAR* var,bool print_readably)
{
    LIST* elt, *oelt;
    char tok[LEXTOKSIZ+1];
    char *buffer = (char *) malloc(BUFSIZE+1);
    bool first = true;

    buffer[0] = '\0';
    buffer[1] = '\0';
    tok[0] = '\0';

    switch (var->type) {
        case S_ROOT:
        case S_LIST:
        case S_VECTOR:
        case S_HASHMAP:
            elt = var->val.lval;
            if (var->type != S_ROOT) strcat(buffer,list_open(var->type));
            while (elt != NULL) {
                strcpy(tok,print_str(elt->var,true));
                if (first) {
                        first = false;
                    }
                else {
                    strcat(buffer," ");
                }
                strcat(buffer,tok);
                elt = elt->next;
            }
            if (var->type != S_ROOT) strcat(buffer,list_close(var->type));
            break;
        case S_INT:
            sprintf(buffer,"%d",var->val.ival);
            break;
        case S_REAL:
            sprintf(buffer,"%f",var->val.rval);
            break;
        case S_STR:
            sprintf(buffer,"\"%s\"",stringify(var->val.pval));
            break;
        case S_SYM:
            sprintf(buffer,"%s",var->val.pval);
            break;
        case S_KEYWORD:
            sprintf(buffer,":%s",var->val.pval);
            break;
        case S_NIL:
            sprintf(buffer,"nil");
            break;
        case S_TRUE:
            sprintf(buffer,"true");
            break;
        case S_FALSE:
            sprintf(buffer,"false");
            break;
        case S_FUN:
            sprintf(buffer,"#<%x>",(unsigned int)var->val.fval);
            break;
        case S_EOF:
            break;
        case S_ERROR:
            sprintf(buffer,"error: %s.",var->val.pval);
            break;
        default:
            sprintf(buffer,"mal: unhandled type: %d",var->type);
    }
    return buffer;
}

/*
 * 	NAME
 *
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
#include "reader.h"
#include "symtab.h"

#define BUFSIZE 1024

char* print_str(VAR* var)
{
    LIST* elt;
    char tok[LEXTOKSIZ+1];
    char *buffer = (char *) malloc(BUFSIZE+1);

    buffer[0] = '\0';
    buffer[1] = '\0';
    tok[0] = '\0';

    switch (var->type) {
        case S_ROOT:
        case S_LIST:
            elt = var->val.lval;
            if (var->type == S_LIST) strcpy(buffer,"(");
            while (elt != NULL) {
                strcpy(tok,print_str(elt->var));
                if (buffer[1] != '\0') strcat(buffer," ");
                strcat(buffer,tok);
                elt = elt->next;
            }
            if (var->type == S_LIST) strcat(buffer,")");
            break;
        case S_INT:
            sprintf(buffer,"%d",var->val.ival);
            break;
        case S_FLOAT:
            sprintf(buffer,"%f",var->val.fval);
            break;
        case S_STR:
            sprintf(buffer,"\"%s\"",var->val.pval);
            break;
        case S_VAR:
            sprintf(buffer,"%s",var->val.pval);
            break;
        case S_KEYWORD:
            sprintf(buffer,":%s",var->val.pval);
            break;
        default:
            sprintf(buffer,"mal: unhandled type: %d",var->type);
    }
    return buffer;
}

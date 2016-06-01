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
#include "env.h"

// FIXME: buffer overruns when printing large lists/functions/vectors/hashes 
#define INITIAL_BUFSIZE 4096

static char* buffer = NULL;
static int buffer_size = INITIAL_BUFSIZE;

char *stringify (char *s)
{
    char *p;

    p = buffer+strlen(buffer);
    *p++ = '"';
    while (*s != '\0') {
        if (*s == '"' || *s == '\\') {
            *p++ = '\\';
        }
        else if (*s == '\n') {
            *p++ = '\\';
            *p++ = 'n';
            s++;
            continue;
        }
        *p++ = *s++;
    }
    *p++ = '"';
    *p = '\0';
    return buffer;
}

char* print_str(VAR* var,bool print_readably,bool top_level)
{
    LIST* elt;
    char tok[INITIAL_BUFSIZE];
    int len, buffer_len,i;
    SYM* sp;
    ITER* iter;

    if (buffer == NULL) {
        buffer = (char *) malloc(INITIAL_BUFSIZE);
        if (buffer == NULL) {
            mal_die("out of memory in print_str");
        }
    }
    if (top_level) {
        *buffer = '\0';
    }
    *tok = '\0';
    switch (var->type) {
        case S_ROOT:
            if (var->val.lval)
                print_str(var->val.lval->var,print_readably,true);
            return buffer;
        case S_VECTOR:
            strcat(buffer,list_open(S_VECTOR));
            if (var->val.vval->size == 0) strcat(buffer," ");
            for (i=0;i<var->val.vval->size;i++) {
                print_str(var->val.vval->vector[i],print_readably,false);
                strcat(buffer," ");
            }
            buffer[strlen(buffer)-1] = '\0'; /* remove trailing space */
            strcat(buffer,list_close(S_VECTOR));
            return buffer;
        case S_HASHMAP:
            strcat(buffer,list_open(S_HASHMAP));
            iter = env_iter_init(var->val.hval);
            sp = env_next(iter);
            if (!sp) strcat(buffer," ");
            while (sp) {
                print_str(str2var(sp->name),true,false);
                strcat(buffer," ");
                print_str(sp->value,true,false);
                strcat(buffer," ");
                sp = env_next(iter);
            }
            buffer[strlen(buffer)-1] = '\0'; /* remove trailing space */
            strcat(buffer,list_close(S_HASHMAP));
            free(iter);
            return buffer;           
        case S_LIST:
            elt = var->val.lval;
            strcat(buffer,list_open(var->type));
            if (elt == NULL) strcat(buffer, " ");
            while (elt != NULL) {
                print_str(elt->var,print_readably,false);
                strcat(buffer," ");
                elt = elt->next;
            }
            buffer[strlen(buffer)-1] = '\0'; /* remove trailing space */
            strcat(buffer,list_close(var->type));
            return buffer;
        case S_INT:
            sprintf(tok,"%ld",var->val.ival);
            break;
        case S_REAL:
            sprintf(tok,"%f",var->val.rval);
            break;
        case S_STR:
            len = strlen(var->val.pval);
            buffer_len = strlen(buffer);
            if (len > (buffer_size - buffer_len)) {
                while (len > (buffer_size - buffer_len)) buffer_size *= 2;
                buffer = realloc(buffer,buffer_size);
                if (buffer == NULL)
                    mal_die("out of memory in print_str (realloc)");
            }
            if (print_readably) {
                stringify(var->val.pval);
            }
            else {
                strcat(buffer,var->val.pval);
            }
            return buffer;
        case S_SYM:
            sprintf(tok,"%s",var->val.pval);
            break;
        case S_KEYWORD:
            sprintf(tok,":%s",var->val.pval);
            break;
        case S_NIL:
            sprintf(tok,"nil");
            break;
        case S_TRUE:
            sprintf(tok,"true");
            break;
        case S_FALSE:
            sprintf(tok,"false");
            break;
        case S_FN:
            sprintf(tok,"#<function %x>",(unsigned int)var->val.fval);
            break;
        case S_BUILTIN:
            sprintf(tok,"#<builtin %x>",(unsigned int)var->val.bval);
            break;
        case S_MACRO:
            sprintf(tok,"#<macro %x>",(unsigned int)var->val.bval);
            break;
        case S_ATOM:
            strcat(buffer,"(atom ");
            print_str(var->val.var,print_readably,false);
            strcat(buffer,")");
            break;
        case S_EOF:
            break;
        case S_ERROR:
            sprintf(tok,"error: %s.",var->val.pval);
            if (print_readably) {
                stringify(tok);
                return buffer;
            }
            break;
        default:
            sprintf(tok,"mal: unhandled type: %d",var->type);
    }
    strcat(buffer,tok);
    return buffer;
}

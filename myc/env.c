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
#include <stdbool.h>

#include "mal.h"
#include "printer.h"
#include "reader.h"
#include "env.h"

ENV* new_env(int size)
{
    ENV* env;
    int i;
    SYM** sym;
    
    env = (ENV*) malloc(sizeof(ENV));
    env->size = size;
    env->sym = (SYM**) malloc(sizeof(SYM*)*size);
    sym = env->sym;
    for (i=0;i<size;i++) {
        *sym++ = NULL;
    }
    return env;
}

int hash(ENV* env,char *s)  
{
	int hashval;
   
	for (hashval=0; *s != '\0'; ) hashval += *s++;
	return (hashval%(env->size));
}

SYM *lookup(ENV* env,char *s)
{
	SYM *sp;
   
	for (sp = env->sym[hash(env,s)];sp != NULL; sp = sp->next) {
		if (strcmp(s,sp->name) == 0) return sp;
    }
	return NULL;
}

ENV* env_put(ENV* env,char *name,VAR* var)
{
	SYM *sp;
	int hashval;
   
	if ((sp=lookup(env,name)) == NULL) {
		sp = (SYM *) malloc(sizeof(SYM));
		if (sp == NULL) return NULL;
        sp->name = strsave(name);
		sp->value = var;
		hashval = hash(env,sp->name);
		sp->next = env->sym[hashval];
		env->sym[hashval] = sp;
	}
	return env;
}

VAR* env_get(ENV* env,char* name)
{
    SYM* sp;
    
	if ((sp=lookup(env,name)) != NULL) {
        return sp->value;
    }
    else {
        return NULL;
    }
}
    
void env_dump(ENV* env)
{
    SYM* sp;
    int i;

    for (i=0;i<env->size;i++) {
        sp = env->sym[i];
        if (sp != NULL) {
            printf("[%d] %s: type %d, value: %s, fun: %x\n",
                   i,sp->name,sp->value->type,print_str(sp->value,true),
                   sp->value->function);
        }
    }        
}

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

ENV* new_env(int size)
{
    ENV* env;

    env = (ENV*) malloc(sizeof(ENV));
    env->size = size;
    env->sym = (SYM*) malloc(sizeof(SYM*)*size);
    for (i=0;i<size;i++) {
        env->sym[i] = NULL;
    }
    return env;
}

int hash(ENV* env,char *s)  
{
	int hashval;
   
	for (hashval=0; *s != '\0'; ) hashval += *s++;
	return (hashval%(env->size));
}

    

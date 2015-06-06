#ifndef CORE_H
#define CORE_H

HASH* ns_get(void);
VAR* cons(VAR*, LIST*);
int count(LIST*);
LIST* copy_list(LIST*);
LIST* deep_copy_list(LIST*);

#endif

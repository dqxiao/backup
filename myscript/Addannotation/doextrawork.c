#include "postgres.h"
#include "addannotation.h"
#include "fmgr.h"
#include "executor/anno_tuple.h"
#include "utils/builtins.h"
#include "utils/supportstring.h"
#include <sys/time.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

int
doextrawork(text * sql){
    char * command;
    
    
    command=text_to_cstring(sql);
   
    
    searchAddWorks(command);
    return 1;
}

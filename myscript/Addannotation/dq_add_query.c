#include "postgres.h"
//#include "addannotation.h"
#include "fmgr.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

int
dq_add_query(text *sql){
    //char * command;
    //AddAnnoQueryPointer aasp;
    
    //command="add annotation on {select * from test where id>3} 'a good product' ;" ;
    
    //aasp=AAparser(command);
    return 1; 
}

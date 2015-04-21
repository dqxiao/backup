#include "postgres.h"
#include "addannotation.h"
#include "fmgr.h"
#include "executor/anno_tuple.h"
#include "utils/builtins.h"
#include "utils/supportstring.h"
#include <stdlib.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

//int mode;

int
addquery(text *sql){
    
    char * command;
    command=text_to_cstring(sql);
    
    AddAnnoQueryPointer aasp=(AddAnnoQueryPointer)malloc(sizeof(AddAnnoQuery));
    /*
    command="add annotation on {select * from test where id<3} 'a good product' " ;
    */
    /**/
    command=replace(command,'\"',"\'"); 
    printf("command:%s \n",command);
    aasp=AAparser(command);
    
    //readSummaryMethodInformation();
    StoreAnnotationContent(aasp);
    StoreAnnotationAssociation(aasp);
    //
    return 1; 
}

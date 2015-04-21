#include "postgres.h"
#include "executor/anno_tuple.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/supportstring.h"
#include <stdlib.h>
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif


int
setPMode(text * inputMode){
	
	// set propagation mode for further use 
	// 
	char * command;
	command=text_to_cstring(inputMode);
	printf("old propagation Mode:%s \n",propagation_mode);
	//printf("command:%s\n",command);
	
	if(strcmp(command,"standard-propagation")==0){
		propagation_mode="standard-propagation";
	}
	if(strcmp(command,"standard")==0) propagation_mode="standard";

	if(strcmp(command,"summary-aware")==0) propagation_mode="summary-aware"; // strategy 2
	if(strcmp(command,"hybrid")==0) propagation_mode="hybrid";
	
	return 1;
	
	//
}
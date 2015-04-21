#ifndef ADDANNOTATION_H
#define ADDANNOTATION_H
#include "nodes/pg_list.h"
#define MaxWait 16
#define Maxlen 1024

/*single one */
typedef struct Config{
    char summaryInstanceName[64];
    char configuration[128];
    char invariant[4];
}Config;
/*complex one*/
typedef struct Catalog{
    char relname[64];
    List * summaryConfigs; // summaryInstanceConfigs 
}Catalog;

/*all the summmary instance information attached to  database*/

typedef struct CaseStmt{
    List * conditions;
    List * whencase;
}CaseStmt;

typedef struct AddAnnoQuery{
    char * targetliststr; // *translate into integer list*/
    char * annotationContent; //* content*/
    char * conditionclause; // conditionalclause;
    char localID[8];  // id
    char * select;
    char * relname; 
}AddAnnoQuery;

typedef struct StoredAddJob{
    char * relname;
    char * localID;
    char * select;
    char * tupleColumns;
    char * annotationContent;
    List * AIDs;
}StoredAddJob;


typedef AddAnnoQuery * AddAnnoQueryPointer; 



extern void readSummaryMethodInformation();
extern List * summaryCatalogs;
extern bool lazy; 

extern AddAnnoQueryPointer AAparser(char *src);
extern void StoreAnnotationContent(AddAnnoQueryPointer aasp);
extern void StoreAnnotationAssociation(AddAnnoQueryPointer aasp);
//extern void StoreAnnotationSummaryResult(AddAnnoQueryPointer aasp);
extern void StoreAnnotationwork(char * selectsql,char * localID,char * tupleColumns,char * relname,char * annotationContent); //stored seperate or single one. // add triggers on this table then it can do this thing for us.
extern void searchAddWorks(char * relname);

#endif 

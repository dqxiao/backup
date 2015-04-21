#include "postgres.h"
#include <stdlib.h>
#include <stdbool.h>
#include "nodes/readfuncs.h"
#include "nodes/parsenodes.h"
#include "nodes/primnodes.h"
#include "parser/parser.h"
#include "parser/parsetree.h"
#include "parser/analyze.h"
#include "tcop/tcopprot.h"
#include "utils/supportstring.h"

#include "executor/spi.h"
#include "utils/builtins.h"
#include "parser/sql_translator.h"
#include "addannotation.h"

#include <sys/time.h>


List * summaryCatalogs=(List *)NULL;
MemoryContext oldcontext = NULL;


bool test=FALSE;
int invariantflag=1;
struct timeval lazyStart;


static void preParser(char * src,AddAnnoQueryPointer aaqp);
static List * getAttrno(Query *query);
static List * singlegetAttrno(List * tl,List * rtable);
static char * listtoarraystring(List * srclist);
static void print_spi_tuple(SPITupleTable * result, int proc);
static void insertCatalogList(char * relname, Config * config);
static void fillSummaryInstance(SPITupleTable * result, int proc);
static char * fillLocalID(SPITupleTable * result, int proc);
static void multifillAIDList(SPITupleTable* result, int proc,List * ajoblist);
static void DostoreAnnotationAssociation(char * selectsql,char * localID,char * targetliststr,char * relname,char * annotationContent);
static void PrintConfig(Config * c);
static void PrintsummaryCatalogs();
static void PrintStoredAddJob(StoredAddJob * ajob);
static void UpdateAID(int curcount);
static List * getSummaryMethodInformation(char * relname);
static void createSummmaryResult(Config * config,List * aidlist,char * annotationContent,int AIDnum);
static void createInvariantSummaryResult(Config * config,List * aidlist,char * annotationContent,int AIDnum);
static char * getSummaryResult(char * sql);
static void updateaddWork(char * relname);
static char * joinListToString(List * srclist,char * comma);
static void getLocalAID(int * AIDstart);
static void DosingleStoreAnnotatinAssociation(StoredAddJob * ajob,int * AIDstart);


static int indexUsage(char * sql);
static double costtime(struct timeval start,struct timeval end);


double
costtime(struct timeval start,struct timeval end){
    double timeuse=1000000*(end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
    return timeuse/1000;
}


int indexUsage(char * sql){
    int ret;
    int proc;
    SPITupleTable * result;
    char * explainsql;
    
    explainsql=malloc((strlen(sql)+20)*sizeof(char));
    
    memset(explainsql,0,(strlen(sql)+20)*sizeof(char));
    
    strcat(explainsql,"explain analyze ");
    
    strcat(explainsql,sql);
    
    
    if(!test){
        
        SPI_connect();
        ret=SPI_exec(explainsql,0);
        proc=SPI_processed;
        
        result=SPI_tuptable;
        
        if(result!=NULL)
        {
            TupleDesc tupdesc = result->tupdesc;
            SPITupleTable *tuptable = result;
            HeapTuple tuple=tuptable->vals[0];
            
            char * planexplain=SPI_getvalue(tuple, tupdesc, 1);
            
            printf("plan explain:%s \n",planexplain);
            
            if(find(planexplain,"index")!=-1){
                
                return 1;
            }
            
        }
        
        
    }
    
    //pfree(explainsql);
    

    return 0;
    
}

void
PrintsummaryCatalogs(){
    ListCell * lf;
    int i=0;
    if(summaryCatalogs==(List *)NULL){
        printf("sorry null \n");
        return; 
    }else{
        //printf("length:%d \n",list_length(summaryCatalogs));
    }
    foreach(lf,summaryCatalogs){
        Catalog * catalog=(Catalog *) lfirst(lf);
        i++;
        printf("relname:%s \n",catalog->relname);
        ListCell * lff;
        foreach(lff,catalog->summaryConfigs){
            Config * c=(Config *)lfirst(lff);
            PrintConfig(c);
        
        }
        printf("----------------------------- \n");
    }
     
}

void
PrintConfig(Config * c){
    
    printf("config->summary_method :%s |",c->summaryInstanceName);
    printf("config->function :%s |",c->configuration);
    printf("config->invariant: %s \n",c->invariant);
    

}

void
PrintStoredAddJob(StoredAddJob * ajob){
    printf("ajob->relname :%s |",ajob->relname);
    printf("ajob->localID :%s |",ajob->localID);
    printf("ajob->select :%s  |",ajob->select);
    printf("ajob->tupleColumns :%s |",ajob->tupleColumns);
    printf("ajob->annotationContent :%s |",ajob->annotationContent);
    printf("ajob->AIDs:%s \n",joinListToString(ajob->AIDs,","));
}


void
PrintStoredAddJobList(List * ajoblist){
    ListCell * lf;
    
    foreach(lf,ajoblist){
        StoredAddJob * ajob=(StoredAddJob *)lfirst(lf);
        PrintStoredAddJob(ajob);
        printf("------------- \n");
    }
    
    
}
void
insertCatalogList(char * relname, Config * config)
{
    bool found; 
    found=false;
    
    if(summaryCatalogs==(List *)NULL)
    {
        found=false;
    }
    else{
        ListCell * lf;
        foreach(lf,summaryCatalogs)
        {
            Catalog * catalog=(Catalog *) lfirst(lf);
            
            
            if(strcmp(catalog->relname,relname)==0)
            {
                
                found=true; 
                catalog->summaryConfigs=lappend(catalog->summaryConfigs,config);

                return; 
            }
             
        }
    }
    
    
    
    if(!found)
    {
        //printf("not found the summmary catalog \n");
        Catalog * catalog=(Catalog *) malloc(sizeof(Catalog));
        strcpy(catalog->relname,relname);
        //printf("relname:%s\n",catalog->relname);
        catalog->summaryConfigs=(List *)NULL;
        catalog->summaryConfigs=lappend(catalog->summaryConfigs,config);
        //printf("length:%d \n",list_length(catalog->summaryConfigs));
        summaryCatalogs=lappend(summaryCatalogs,catalog);
        return;
    }
    
    
    
}

void
PrintCaseStmt(CaseStmt * casestmt){
    ListCell * lf;
    
    printf("condition: ");
    foreach(lf,casestmt->conditions){
        char * condition=(char *)lfirst(lf);
        
        printf(" %s ",condition);
    }
    printf("||");
    
    printf("whencase:");
    
    foreach(lf,casestmt->whencase){
        char * whencase=(char *)lfirst(lf);
        
        printf(" %s ",whencase);
    }
    
    printf("\n");
}

void
PrintCaseStmtList(List * caseStmtList){
    ListCell * lf;
    
    foreach(lf,caseStmtList){
        //
        CaseStmt * casestmt=(CaseStmt *)lfirst(lf);
        
        PrintCaseStmt(casestmt);
        
    }
}


List *
fillAIDlist(SPITupleTable * result,int proc){
    List * aidlist=(List *)NULL;
    if(result!=NULL){
        int j;
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable = result;
        
        for(j=0;j<proc;j++){
            char * attrval;
            HeapTuple tuple=tuptable->vals[j];
            
            attrval=SPI_getvalue(tuple, tupdesc, 1);
            
            //printf("val :%s \n",attrval);
            
            aidlist=lappend(aidlist,attrval);
            
        }
    }else{
        printf("nothing \n");
    }
    
    return aidlist;
}


void
multifillAIDList(SPITupleTable* result, int proc,List * ajoblist)
{
    
    //List * aidlist=(List *)NULL;
    if(result!=NULL)
    {
        int j;
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable = result;
        
        for(j=0;j<proc;j++)
        {
            char * attrval;
            char * caseval;
            HeapTuple tuple=tuptable->vals[j];
            
            attrval=SPI_getvalue(tuple, tupdesc, 1);
            
            
            
            
            caseval=SPI_getvalue(tuple,tupdesc,2);
            
                       
            if(caseval!=(char *)NULL){
                char * * dest;
                int num;
                dest=(char **)malloc(64*sizeof(char *));
                num=split(dest,caseval,"+");
                
                for(int i=0;i<num;i++){
                    //printf("%s |",dest[i]);
                    
                    ListCell * lff;
                    
                    foreach(lff,ajoblist)
                    {
                        StoredAddJob * ajob=(StoredAddJob *)lfirst(lff);
                        
                        if(strcmp(ajob->localID,dest[i])==0){
                            
                            ajob->AIDs=lappend(ajob->AIDs,attrval);
                            
                            break;
                    }
                        
                    
                }
             }
            }
        
        }
    
        
        //
        
    }
    
    
   
    
}


char * 
joinListToString(List * srclist,char * comma){
    
    int length;
    ListCell * lf;
    int count;
    char *final;
    length=list_length(srclist);
    
    
    final=(char *)malloc(length*16);
    
    memset(final,0,length*16);
    count=1;
    
    foreach(lf,srclist)
    {
        char * single=(char *)lfirst(lf);
        
        
        strcat(final,single);
        if(count!=length){
            strcat(final,comma);
        }
        count+=1;
    
    }

    return final;

}

char * whereClauseMaker(List * ajoblist){
    /**/
    ListCell * lf;
    int length=list_length(ajoblist);
    
    char * target=(char *)malloc(length*64);
    memset(target,0,length*64);
    
    /**/
    int index=0;
    foreach(lf,ajoblist){
        
        StoredAddJob * ajob=(StoredAddJob *)lfirst(lf);
        char * select=ajob->select;
        char * curcondition=get_substr(select,find(select,"where")+5,strlen(select));
        /**/
        
        strcat(target,curcondition);
        strcat(target," ");
        
        if(index<length-1){
            strcat(target,"or");
        }
        
        index+=1;
    }
    
    return target;
    
}

char * multiQueryNew(List * ajoblist){
    ListCell * lf;
    int llength;
    int count=1;
    char * result;
    char singlepart[32];
    llength=list_length(ajoblist);
    result=malloc(llength*32);
    
    // support simple statement 
    foreach(lf,ajoblist){
        StoredAddJob * ajob=(StoredAddJob *)lfirst(lf);
        char * select=ajob->select;
        
        char *curcondition=get_substr(select,find(select,"where")+5,strlen(select));
        
        //printf("condition:%s \n",curcondition);
        memset(singlepart,0,32);
        sprintf(singlepart,"%s->%s",curcondition,ajob->localID);
        strcat(result,singlepart);
        
        if(count!=llength){
            strcat(result,",");
        }
    }
    
    return result;
}


char *
multiQuery(List * ajoblist){
    
    List * caseStmtList=(List *)NULL;
    ListCell * lf;
    ListCell * stmtlf;
    int stmtlength;
    char * finalclause;
    
    foreach(lf,ajoblist){
        
        StoredAddJob * ajob=(StoredAddJob *)lfirst(lf);
        
        /*
         */
        char * select=ajob->select;
       
        char * curcondition=get_substr(select,find(select,"where")+5,strlen(select));
     
        CaseStmt * newcasestmt=(CaseStmt*) malloc(sizeof(CaseStmt));
        
        if(caseStmtList!=(List *)NULL)
        {
            ListCell * lff;
            
            List * newcasestmtlist=(List *)NULL;
            
            foreach(lff,caseStmtList)
            {
                
                CaseStmt * casestmt=(CaseStmt *)lfirst(lff);
                // yuanben 
                
                newcasestmt->conditions=list_copy(casestmt->conditions);
                newcasestmt->conditions=lappend(newcasestmt->conditions,curcondition);
                
                newcasestmt->whencase=list_copy(casestmt->whencase);
                newcasestmt->whencase=lappend(newcasestmt->whencase,ajob->localID);
                
                
                
                /**/
                newcasestmtlist=lappend(newcasestmtlist,newcasestmt);
                
                newcasestmtlist=lappend(newcasestmtlist,casestmt);
                
                
                
            }
            
            caseStmtList=newcasestmtlist;
            
            CaseStmt * curNewCaseStmt=(CaseStmt *) malloc(sizeof(CaseStmt));
            curNewCaseStmt->conditions=(List *)NULL;
            curNewCaseStmt->whencase=(List *)NULL;
            curNewCaseStmt->conditions=lappend(curNewCaseStmt->conditions,curcondition);
            curNewCaseStmt->whencase=lappend(curNewCaseStmt->whencase,ajob->localID);
            
            caseStmtList=lappend(caseStmtList,curNewCaseStmt);
                        
            
        
        }
        else{
            //printf("NULL list \n");
            CaseStmt * curNewCaseStmt=(CaseStmt *) malloc(sizeof(CaseStmt));
            curNewCaseStmt->conditions=(List *)NULL;
            curNewCaseStmt->whencase=(List *)NULL;
            curNewCaseStmt->conditions=lappend(curNewCaseStmt->conditions,curcondition);
            curNewCaseStmt->whencase=lappend(curNewCaseStmt->whencase,ajob->localID);
            
            caseStmtList=lappend(caseStmtList,curNewCaseStmt);
        }
    }

    //PrintCaseStmtList(caseStmtList);
    
    stmtlength=list_length(caseStmtList);
    
    finalclause=(char *)malloc(stmtlength*300);
    memset(finalclause,0,stmtlength*300);
    
    char *singlestmt=(char *)malloc(32*16);

    
    foreach(stmtlf,caseStmtList){
        //char * singlestmt;
        CaseStmt * casestmt=(CaseStmt *)lfirst(stmtlf);
        
        /**/
        //int length=list_length(casestmt->conditions);
        char *conditions=joinListToString(casestmt->conditions," and ");
        char *whencase=joinListToString(casestmt->whencase,"+");
        
        
        
        /**/
        //printf("conditions:%s \n",conditions);
        
        memset(singlestmt,0,32*16);
        strcat(singlestmt,"when ");
        strcat(singlestmt,conditions);
        strcat(singlestmt," then ");
        strcat(singlestmt,"\'");
        strcat(singlestmt,whencase);
        strcat(singlestmt,"\'");
        strcat(singlestmt,"\n");
        
       
        
        strcat(finalclause,singlestmt);
        
        
        
        
    }
    
    

    
    
    
    return finalclause;
    //return "ok";
}


void
DoMultiStoreAssociation(List * ajoblist){
    
    
   
    
    ListCell * lf;
    StoredAddJob * ajob;
    char * relname;
    char * selectFormate;
    char * selectsql;
    int length;
    int AIDstart;
    ListCell * lff;
    
    
    

    length=list_length(ajoblist);
    lf=list_head(ajoblist);
    ajob=(StoredAddJob *)lfirst(lf);
    relname=ajob->relname;
    
    struct timeval cqStart,cqEnd;
    gettimeofday(&cqStart,0);
    char * valquery=multiQueryNew(ajoblist);
    //char * multiquery=multiQuery(ajoblist);
    char * target=whereClauseMaker(ajoblist);
    /**/
    selectFormate="select id,multiEval(%s.*,\'%s\') from %s where %s";
    selectsql=(char *)malloc(length*64+256);
    sprintf(selectsql,selectFormate,relname,valquery,relname,target);
    gettimeofday(&cqEnd,0);
    printf(" Time usage of creating query:%f \n",costtime(cqStart,cqEnd));
    
  
    
    if(!test){
        int ret;
        int proc;
        SPITupleTable * result;
        
        struct timeval eqStart,eqEnd;
        gettimeofday(&eqStart,0);
        
        SPI_connect();
        ret=SPI_exec(selectsql,0);
        proc=SPI_processed;
        result=SPI_tuptable;
        
        gettimeofday(&eqEnd,0);
        
        printf("time usage of executing compound query :%f ms\n",costtime(eqStart,eqEnd));
        
        
        struct timeval prStart,prEnd;
        gettimeofday(&prStart,0);
        multifillAIDList(result,proc,ajoblist);
        gettimeofday(&prEnd,0);
        
        printf("time usage of parsing compound query's result : %f ms \n",costtime(prStart,prEnd));
        
    }

    
    getLocalAID(&AIDstart);
    
    

    foreach(lff,ajoblist){
        StoredAddJob * ajob=(StoredAddJob *)lfirst(lff);
        DosingleStoreAnnotatinAssociation(ajob,&AIDstart);
    }
    //pfree(ajoblist);
}





void multifromTableStore(SPITupleTable * result, int proc){
    
    
    if(result!=NULL)
    {
        
        int i,j;
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable= result;
        
        List * ajoblist=(List *)NULL;
        
        
        
        /*summary Instance*/
        for(j=0;j<proc;j++)
        {
            HeapTuple tuple;
            StoredAddJob * ajob;
            ajob=malloc(sizeof(StoredAddJob));
            
            ajob->AIDs=(List *)NULL;// add
            tuple=tuptable->vals[j];
        
            
            for(i=1;i<=tupdesc->natts;i++)
            {
                char * attrname;
                char * attrval;
                attrname=SPI_fname(tupdesc,i);
                attrval=SPI_getvalue(tuple, tupdesc, i);
                //printf("%s:%s | ",attrname,attrval);
                /**/
                
                if(strcmp(attrname,"relname")==0) {ajob->relname=attrval;continue;}
                if(strcmp(attrname,"localid")==0) {ajob->localID=attrval;continue;}
                if(strcmp(attrname,"query")==0)  {ajob->select=attrval;continue;}
                if(strcmp(attrname,"tuplecolumns")==0) {ajob->tupleColumns=attrval;continue;}
                if(strcmp(attrname,"annotationcontent")==0){ajob->annotationContent=attrval; continue;}
                
                
            }
            //printf("\n");
            ajoblist=lappend(ajoblist,ajob);
            
            
            
            //
            
            
        }
        
        
        //pfree(TupleDesc);
        //pfree(tuptable);
        DoMultiStoreAssociation(ajoblist);
        
        
    }

}

void
fromTableStore(SPITupleTable * result, int proc)
{
    if(result!=NULL)
    {
        int i,j;
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable= result;
        
        
        /*summary Instance*/
        for(j=0;j<proc;j++)
        {
            HeapTuple tuple;
            StoredAddJob * ajob;
            ajob=malloc(sizeof(StoredAddJob));
            tuple=tuptable->vals[j];
            //printf("come here \n")
            for(i=1;i<=tupdesc->natts;i++)
            {
                char * attrname;
                char * attrval;
                
                
                attrname=SPI_fname(tupdesc,i);
                attrval=SPI_getvalue(tuple, tupdesc, i);
                
                               
                
                
                if(strcmp(attrname,"relname")==0) {ajob->relname=attrval;continue;}
                if(strcmp(attrname,"localid")==0) {ajob->localID=attrval;continue;}
                if(strcmp(attrname,"query")==0)  {ajob->select=attrval;continue;}
                if(strcmp(attrname,"tuplecolumns")==0) {ajob->tupleColumns=attrval;continue;}
                if(strcmp(attrname,"annotationcontent")==0){ajob->annotationContent=attrval; continue;}
                
                
            }
            //printf("\n");
            //PrintStoredAddJob(ajob);
            //DostoreAnnotationAssociation(char * selectsql,char * localID,char * targetliststr,char * relname)
            DostoreAnnotationAssociation(ajob->select,ajob->localID,ajob->tupleColumns,ajob->relname,ajob->annotationContent);
            // here,we are quit lazy;
            
            
        }
    }
}

char * 
fillLocalID(SPITupleTable * result, int proc){
    if(result!=NULL){
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable = result;
        HeapTuple tuple=tuptable->vals[0];
        
        return SPI_getvalue(tuple, tupdesc, 1);
        //printf("localID :%s",localID);
    }
    else{
        printf("somthing wrong with geting localID");
    }
}

void
fillSummaryInstance(SPITupleTable * result, int proc)
{
   
    if(result!=NULL){
        
        TupleDesc tupdesc = result->tupdesc;
        
        SPITupleTable *tuptable;
        //=malloc(sizeof(SPITupleTable));
        tuptable= result;
        int i,j;
        /*summary Instance*/
        for(j=0;j<proc;j++){
            char * tableName;
            Config * config;
           
            HeapTuple tuple=tuptable->vals[j];
            
            config=(Config *)malloc(sizeof(Config));
            
         
            for(i=1;i<=tupdesc->natts;i++)
            {
                char * attrname;
                char * attrval;
                
               
                attrname=SPI_fname(tupdesc,i);
                
                attrval=SPI_getvalue(tuple, tupdesc, i);
                
                if(strcmp(attrname,"table_name")==0) tableName=attrval;
                if(strcmp(attrname,"summary_method")==0) 
                    strcpy(config->summaryInstanceName,attrval);
                if(strcmp(attrname,"function")==0)
                    strcpy(config->configuration,attrval);
                if(strcmp(attrname,"invariant")==0)
                    strcpy(config->invariant,attrval);
                
                
                /**/
                
                
            }
            insertCatalogList(tableName, config);
        }
    
    }
    
}

void
print_spi_tuple(SPITupleTable * result, int proc){
    
    
    if (result != NULL)
    {
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable = result;
        char buf[8192];
        int i, j;
        
        for (j = 0; j < proc; j++)
        {
            HeapTuple tuple = tuptable->vals[j];
            
            for (i = 1, buf[0] = 0; i <= tupdesc->natts; i++){
                snprintf(buf + strlen (buf), sizeof(buf) - strlen(buf), " %s%s",
                         SPI_getvalue(tuple, tupdesc, i),
                         (i == tupdesc->natts) ? " " : " |");
               
            }
             elog(INFO, "EXECQ: %s", buf);
        }
    }
    
}

//static AddAnnoQueryPointer AAparser(char * src);

char *
listtoarraystring(List * srclist)
{
    ListCell * lf;
    char * astring=(char *)malloc(128*sizeof(char));
    char numstr[3];
    strcat(astring,"{");
    foreach(lf,srclist)
    {
        int num;
        memset(numstr,0,3);
        num=(int)lfirst(lf);
        sprintf(numstr,"%d",num);
        strcat(astring,numstr);
        strcat(astring,",");
    }
    astring=get_substr(astring,0,strlen(astring)-2);
    strcat(astring,"}");
    
    return astring;
}

List *
singlegetAttrno(List * tl,List * rtable){
    List * result;
    ListCell * lf;
    
    
    result=(List *)NULL;
    
    foreach(lf,tl)
    {
        Node * lfn=(Node *)lfirst(lf); // node
        
        TargetEntry * tle=(TargetEntry *) lfn; // targetentry
        Node * expr=(Node *)tle->expr;
        
        if(expr){
            
            switch(nodeTag(expr))
            {
                case T_Var:
                {
                    Var *var=(Var *)expr;
                    char * relname;
                    char * attname;
                    int varno;
                    int varattno;
                    
                    //List * lattrno;
                    
                    RangeTblEntry *rte;
                    Assert(var->varno > 0 &&
                           (int) var->varno <= list_length(rtable));
                    
                    varno=(int)var->varno;
                    varattno=(int)var->varattno;
                    rte = rt_fetch(var->varno, rtable);
                    relname = rte->eref->aliasname;
                    attname = get_rte_attribute_name(rte, var->varattno);
                    
                    //printf("%s.%s \n", relname, attname);
                    //printf("varno:%d,varattno:%d \n",var->varno,var->varattno);
                    
                    //
                    
                    result=lappend_int(result,var->varattno);
                    
                    break;
                }
            }
            
        }
        
    }
    //printf("result :%d \n",list_length(result));
    return result;
}


List * 
getAttrno(Query *query){
    
    List * rtable;
    List * tl;
    rtable=query->rtable;
    tl=query->targetList;
    
    return singlegetAttrno(tl,rtable);
    
}






void
preParser(char *src, AddAnnoQueryPointer aaqp){
    
    char *s;
    char *p;
    int curLen;
    int flag;
    char * select;
    int start,end;
    int index=0;
    
    
    s=(char*)malloc(Maxlen*sizeof(char));
    p=src;
    select=(char*)malloc(Maxlen*sizeof(char));
    flag=0; 
    start=end=-1;
    
    
    while(p &&sscanf(p,"%s",s)==1){
        curLen=(int)strlen(s);
        s[curLen]='\0';
        p+=curLen;
        p++;
        if(strcmp(s,"add")==0) flag=1;
        if(strcmp(s,"annotation")==0 &&flag==1) flag+=1;
        if(strcmp(s,"on")==0 && flag==2) {flag+=1; break;}
    }
    
    
    if(flag!=3){
        printf("systax error \n");
        return;
    }
    
    while(*p==' ') p++;
    
    
    while(*p!='\0' &&end !=0)
    {
        switch(*(p))
        {
            case '{':
            {
                start=0;
                break;
            }
            case '}':
            {
                end=0;
                //printf("End index :%d",index);
                select[index]='\0';
                break;
            }
            default:
            {
                if(start==0 && end==-1)
                {
                    select[index]=*p;
                    index++;
                }
                break;
            }
        }
        p++; 
    }
    aaqp->select=select;
    
    
    
    aaqp->annotationContent=replace(p,'\'',"");
    
    
    //debuging
    
    //printf("select query: %s \n",aaqp->select);
    //printf("annotation: %s\n",aaqp->annotationContent);
}

AddAnnoQueryPointer
AAparser(char *src){
    
    bool valid;
    List * rawParseTree;
    ListCell * lf;
    char * query; 
    AddAnnoQueryPointer aaqp; 
    
    valid=false;
    
    
    
    aaqp=(AddAnnoQueryPointer)malloc(sizeof(AddAnnoQuery));
    
    
    preParser(src,aaqp);
    
    query=aaqp->select;
    rawParseTree=pg_parse_query(query);

    
    if(list_length(rawParseTree)==1){
        // only include one select stmt;
        //
        lf=list_head(rawParseTree);
        Node * parsetree=(Node *)lfirst(lf);
        if(IsA(parsetree,SelectStmt)){
            SelectStmt * stmt;
            List * fromclause;
            
            stmt=(SelectStmt *) parsetree;
            fromclause=stmt->fromClause;
            
            int relnum=list_length(fromclause);
            if(relnum==1&& !stmt->groupClause){
                valid=true;
            }
            
            
            
            if(valid){
                //printf("now we need do something.\n");
                //
                Query * pquery;
                List * targetlist;
                ListCell * lf;
                RangeVar * rv;
                
                
                
                lf=list_head(fromclause);
                rv=(RangeVar *)lfirst(lf);
                aaqp->relname=rv->relname;
                ///printf("relname:%s \n",rv->relname);
                targetlist=(List *)NULL;

                pquery=parse_analyze(parsetree, query,NULL, 0);
                targetlist=getAttrno(pquery);
                
                aaqp->targetliststr=listtoarraystring(targetlist);
                //printf("targetlist: %s \n",aaqp->targetliststr);
                
            }

        }
    }
    
    return aaqp; 
}



List *
getSummaryMethodInformation(char * relname)
{
    ListCell * lf;
    
    foreach(lf,summaryCatalogs){
        Catalog * catalog=(Catalog *) lfirst(lf);
        if (strcmp(catalog->relname,relname)==0) {
            //printf("exist summary instance \n");
            return catalog->summaryConfigs; 
        }
    }
    return (List *)NULL;
    
}

void
readSummaryMethodInformation(){
    int ret;
    int proc;
    
    summaryCatalogs=(List *)NULL;
    SPITupleTable * result;
    
    char * command="select * from summary_catalog sc, summary_methods_config smc where sc. summary_method=smc.summaryintance_id ";
    
    
    SPI_connect();
    ret=SPI_exec(command,0);
    proc=SPI_processed;
   
    result=SPI_tuptable;
    fillSummaryInstance(result, proc);
    
    
    //pfree(command);
    //pfree(command);
    //pfree(result);
    //pfree(result);
    
    
}

void
updateID(char *localID){
    int ret;
    int proc;
    int id;
    char * updateformate="select setval('anno_table_seq',%d)";
    char updatesql[128];

    id=atoi(localID);
    
    //
    sprintf(updatesql,updateformate,id+1);
    
    SPI_connect();
    
    ret=SPI_exec(updatesql,0);
    proc=SPI_processed;
    
    printf("update local id ok \n");
    
     
}

void
StoreAnnotationContent(AddAnnoQueryPointer aasp){
    char * insertsqlFormate;
    char * localIdsql;
    char localID[8];
    char insertsql[Maxlen];
    
    int ret;
    int proc;
    //char * incsql;
    char * incsqlformate;
    
    localIdsql="select last_value from anno_table_seq";
    SPITupleTable * result;
    
    SPI_connect();
    ret=SPI_exec(localIdsql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    strcpy(localID,fillLocalID(result,proc));
    
   
    strcpy(aasp->localID,localID);
    
    
   
    
    insertsqlFormate="insert into anno_table (id,value) values (%s,\'%s\')";
    
    sprintf(insertsql,insertsqlFormate,localID,aasp->annotationContent);
    
    
    if(!test){
        SPI_connect();
        ret=SPI_exec(insertsql,0);
        proc=SPI_processed;
        if(proc==1){
            printf("insert ok annotation content ok \n");
            updateID(localID);
        }
        
        
    }
    
         
    
     
    //SPI_finish();
}


char *
getSummaryResult(char * sql){
    int ret;
    int proc;
    SPITupleTable * result;
    
    // single result
    SPI_connect();
    ret=SPI_exec(sql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    
    if(result!=NULL){
        TupleDesc tupdesc = result->tupdesc;
        SPITupleTable *tuptable = result;
        HeapTuple tuple=tuptable->vals[0];
        
        return SPI_getvalue(tuple, tupdesc, 1);
    }
    else{
        printf("something wrong with get summary result :%s \n",sql);
    }
    
    return (char *)NULL;
    
}

void
createVariantSummaryResult(Config * config,List * aidlist,char * annotationContent,int AIDnum){
    char * result;
    char * summaryInstanceName;
    char * summaryformate;
    char * insertformate;
    char summarySql[2*Maxlen];
    ListCell * lf;
    int count=0;
    
    insertformate="insert into summary_result(id,summary_method,result) values (%s,\'%s\',\'%s\')";  
    // summaryInstanceName: methods 
    summaryInstanceName=config->summaryInstanceName;
    if(find(summaryInstanceName,"classifier")!=-1){
        summaryformate="select classify(\'%s\',\'%s\')";
        
    
        
    }
    if(find(summaryInstanceName,"snippet")!=-1){
        summaryformate="select snippet(\'%s\',\'%s\')";
        
    }

    int aidlength=list_length(aidlist);
    
    /**/
    sprintf(summarySql,summaryformate,summaryInstanceName,annotationContent);
    
    struct timeval msStart,msEnd;
    gettimeofday(&msStart,0);
    for(int i=0;i<aidlength;i++){
        result=getSummaryResult(summarySql);
    }
    gettimeofday(&msEnd,0);
    
    printf("for this summary method, we use time %f ms \n",costtime(msStart,msEnd));
    
    char aid[8];
    char insertsql[Maxlen];
    
    foreach(lf,aidlist){
        
        int ret;
        int proc;
        /*to get the result */
        //
        
        
        //result=getSummaryResult(summarySql);
        
        sprintf(aid,"%d",AIDnum+count);
        sprintf(insertsql,insertformate,aid,summaryInstanceName,result);
        
      
        
        if(!test){
            SPI_connect();
            ret=SPI_exec(insertsql,0);
            proc=SPI_processed;
            if(proc==1){
                //printf("insert into summary result :%s",insertsql);
            }
        }
        
        count+=1;
    }

    
}


void
createInvariantSummaryResult(Config * config,List * aidlist,char * annotationContent,int AIDnum){
    
    char * result;
    char * summaryInstanceName;
    char * insertformate;
    char * summaryformate;
    char summarySql[2*Maxlen];
    ListCell * lf;
    int count=0;
    
    insertformate="insert into summary_result(id,summary_method,result) values (%s,\'%s\',\'%s\')";
    summaryInstanceName=config->summaryInstanceName;
    
      
    if(find(summaryInstanceName,"classifier")!=-1){
        summaryformate="select classify(\'%s\',\'%s\')";
    
        
    }
    if(find(summaryInstanceName,"snippet")!=-1){
         summaryformate="select snippet(\'%s\',\'%s\')";
         //printf("Do snippet \n");
    }
    
    
    sprintf(summarySql,summaryformate,summaryInstanceName,annotationContent);
    
    
    
    // here just for convienece, let us check whether it is true
    //struct timeval sStart,sEnd;
    //gettimeofday(&sStart,0);
    result=getSummaryResult(summarySql);
    //gettimeofday(&sEnd,0);
    
    //printf("for single get summary result we use :%f ms \n",costtime(sStart,sEnd));

    
    
    
    
    
    /**/
    char aid[8];
    char insertsql[Maxlen];
    
    foreach(lf,aidlist){
        
        int ret;
        int proc;
        sprintf(aid,"%d",AIDnum+count);
        sprintf(insertsql,insertformate,aid,summaryInstanceName,result);
        
        //printf("insert sql:%s \n",insertsql);
        
        if(!test){
            SPI_connect();
            ret=SPI_exec(insertsql,0);
            proc=SPI_processed;
            if(proc==1){
                //printf("insert into summary result :%s",insertsql);
            }
        }
        
        count+=1;
    }
    
    
    //gettimeofday(&storeend,0);
    
    //double storetimeuse  = 1000000*(storeend.tv_sec - storestart.tv_sec) + storeend.tv_usec - storestart.tv_usec;
    
    //printf("for storage we use time:%f ms\n",storetimeuse/1000);
    
}


void
createSummaryResult(Config * config,List * aidlist,char * annotationContent,int AIDnum){
    char * invariant;
    invariant=config->invariant;
    if(strcmp(invariant,"t")==0 && invariantflag==1){
        
        createInvariantSummaryResult(config,aidlist,annotationContent,AIDnum);
    }
    else{
        printf("not vairant \n");
        createVariantSummaryResult(config,aidlist,annotationContent,AIDnum);
    }
    
}

void
UpdateAID(int curcount){
    
    char updateSql[128];
    int ret;
    int proc;
    char * updateformate="select setval('data_anno_seq',%d)";
    
    sprintf(updateSql,updateformate,curcount);
    //printf("update sql: %s \n",updateSql);
   
    if(!test){
        SPI_connect();
        ret=SPI_exec(updateSql,0);
        proc=SPI_processed;
        if(proc==1){
        }
    }
    
}

void
getLocalAID(int * AIDstart){
    
    /**/
    int ret;
    int proc;
    char AID[12];
    SPITupleTable * result;
    char * selectAIDsql;
    
    selectAIDsql="select last_value from data_anno_seq";
    
    SPI_connect();
    ret=SPI_exec(selectAIDsql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    
    strcpy(AID,fillLocalID(result,proc));
    
    //printf("AID:%s \n",AID);
    
    
    /**/
    *AIDstart=atoi(AID);
    
    
    
    

}


void
DosingleStoreAnnotatinAssociation(StoredAddJob * ajob,int * AIDstart){
    
    /**/
    int start=*AIDstart;
    char * insertASFormate;
    ListCell * lf;
    ListCell * slf;
    List * summaryConfigs=(List *)NULL;
    
    /**/
    
    insertASFormate="insert into data_anno (id,aid,table_name,tuple_id,tuple_column) values(%s,%d,\'%s\',%s,\'%s\')";
    char insertsql[Maxlen];
    
    foreach(lf,ajob->AIDs){
        char *tupleid=(char *)lfirst(lf);
        
        
    
        
        sprintf(insertsql,insertASFormate,ajob->localID,start,ajob->relname,tupleid,ajob->tupleColumns);
        
        printf("insert sql :%s \n",insertsql);
        
        
        if(!test){
            int ret;
            int proc;
            
            SPI_connect();
            ret=SPI_exec(insertsql,0);
            proc=SPI_processed;
            if(proc==1){
                printf("insert into data_anno successfully \n");
            }
            
        }
        
        
        
        start+=1;
        
        
    }
    
    //printf("start:%d |",start);
    //printf("AIDstart:%d \n",*AIDstart);
    
    
    summaryConfigs=getSummaryMethodInformation(ajob->relname);
    
    
    foreach(slf,summaryConfigs){
        Config * config=(Config *)lfirst(slf);
        createSummaryResult(config,ajob->AIDs,ajob->annotationContent,*AIDstart);
        
    }
    
    
       
    

    *AIDstart=start;
    
    UpdateAID(*AIDstart);
    
    
    
}

void
DostoreAnnotationAssociation(char * selectsql,char * localID,char * targetliststr,char * relname,char * annotationContent)
{
    int ret;
    int proc;
    char AID[Maxlen];
    SPITupleTable * result;
    List * aidlist;
    ListCell * lf;
    ListCell * slf;
    int AIDnum; 
    char * insertASFormate;
    char * selectAIDsql="select last_value from data_anno_seq";
    int count=0;
    int j;
    List * summaryConfigs=(List *)NULL;
    
    
    printf("Exe selectAIDsql:%s \n",selectAIDsql);
    
    SPI_connect();
    ret=SPI_exec(selectAIDsql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    strcpy(AID,fillLocalID(result,proc));
    
    
    printf("Exe selectSql:%s \n", selectsql);
    SPI_connect();
    ret=SPI_exec(selectsql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    aidlist=(List *)NULL;
    aidlist=fillAIDlist(result,proc);
    
    
    AIDnum=atoi(AID);
    insertASFormate="insert into data_anno (id,aid,table_name,tuple_id,tuple_column) values(%s,%s,\'%s\',%s,\'%s\')";
    
    char insertsql[Maxlen];
    char aid[12];
    
    foreach(lf,aidlist){
        char *tupleid=(char *)lfirst(lf);
        sprintf(aid,"%d",AIDnum+count);
        sprintf(insertsql,insertASFormate,localID,aid,relname,tupleid,targetliststr);
        printf("Exe insertsql:%s \n",insertsql);
        if(!test){
            SPI_connect();
            ret=SPI_exec(insertsql,0);
            proc=SPI_processed;
            
            if(proc==1){
                printf("insert assocaitation into data_anno \n");
            }
            //SPI_finish();
        }

       
        count+=1;
        
    }
    
    UpdateAID(AIDnum+count);
    
    
    
    
    summaryConfigs=getSummaryMethodInformation(relname);
    
    
    foreach(slf,summaryConfigs){
        Config * config=(Config *)lfirst(slf);
        createSummaryResult(config,aidlist,annotationContent,AIDnum);
        
    }
    
}

// store separate or together, this is one question, it depends on your! 
void
StoreAnnotationwork(char * selectsql,char * localID,char * tupleColumns,char * relname,char * annotationContent)
{
    char * storeWorkFormate;
    int ret;
    int proc;
    char storeWorkSql[2*Maxlen];
    
    memset(storeWorkSql,0,2*Maxlen);
    

    /**/
    
    storeWorkFormate="insert into addwork values ('\%s\',%s,\'%s\',\'%s\',\'%s\')";
    
    /*
    printf("* selectsql:%s\n",selectsql);
    printf("* relname:%s \n",relname);
    printf("* localID:%s \n",localID);
    printf("* tupleColumns:%s \n",tupleColumns);
    printf("* annotationContent:%s \n",annotationContent);
    
    printf("* formate:%s\n",storeWorkFormate);
    */
    
    sprintf(storeWorkSql,storeWorkFormate,relname,localID,selectsql,tupleColumns,annotationContent);
    
    
    
    //printf("store work sql: %s \n", storeWorkSql);
    if(!test){
        SPI_connect();
        ret=SPI_exec(storeWorkSql,0);
        proc=SPI_processed;
        //result=SPI_tuptable;
        
        if(proc==1){
            //printf("Insert: insert into addwork sucessfully \n");
        }
    }
    
    
}
  

void updateaddWork(char *relname){
    int ret;
    int proc;
    char * formate="delete from addwork where relname=\'%s\' ";
    char updatesql[Maxlen];
    
    sprintf(updatesql,formate,relname);
    
    //printf("update add work sql:%s \n",updatesql);
    
    
    SPI_connect();
    ret=SPI_exec(updatesql,0);
    proc=SPI_processed;
    
    printf("update add work successfull \n");
    
    //SPI_finish();
    
}

void
searchAddWorks(char * relname)
{
    struct timeval qcStart,qcEnd;
    gettimeofday(&lazyStart,0); //set Lazy start time 
    gettimeofday(&qcStart,0);
    int ret,proc;
    SPITupleTable * result;
    char selectsql[2*Maxlen];
    
    char * selectWorkFormate="select * from addwork where relname=\'%s\'";
    
    sprintf(selectsql,selectWorkFormate,relname);
    
    SPI_connect();
    ret=SPI_exec(selectsql,0);
    proc=SPI_processed;
    result=SPI_tuptable;
    gettimeofday(&qcEnd,0);
    printf("Time usage of search addwork :%f ms \n",costtime(qcStart,qcEnd));
    
    
    
    
    if(proc!=0){
        // here we can do the same job as we ever done in store
         // need one SPI_finish();
        struct timeval readEnd,readStart;
        gettimeofday(&readStart,0);
        readSummaryMethodInformation();
        gettimeofday(&readEnd,0);
        printf("time usage of reading summary methods information :%f ms \n",costtime(readStart,readStart));
        
    
        
    
        
        multifromTableStore(result,proc);
    }
    else{
        printf("Oh it is good, forget it \n");
    }
    
    
    updateaddWork(relname); //after we make sure everything is ok.
    struct timeval lazyEnd;
   
    gettimeofday(&lazyEnd,0);
    
    printf("time usage of finish all tasks :%f ms \n",costtime(lazyStart,lazyEnd));
    
    SPI_finish();
    //SPI_finish();
    //SPI_finish();
    
    
    
}


void StoreAnnotationAssociation(AddAnnoQueryPointer aasp){
    
    char * selectIDformate;
    char *select;
    int start;
    char selectsql[Maxlen];
    int lazyflag;
    
    selectIDformate="select id %s";

    select=aasp->select;
    printf("after parsing select query:%s #\n",select);
    
    start=find(select,"from");
    
    sprintf(selectsql,selectIDformate,select+start);
    
    
    lazyflag=1;
    
    if(strcmp(addannotation_lazy_mode,"T")==0 &&lazyflag==0){
        printf("**** for lazy working, we just store the information **** \n");
        // we just store the adding annotation work into systemic table
        StoreAnnotationwork(selectsql,aasp->localID,aasp->targetliststr,aasp->relname,aasp->annotationContent);
    }
    else{
        if(lazyflag==1)
        {
            
            printf("*** for eager working, we do store assocation and summary right now *** \n");
            readSummaryMethodInformation();
            printf("readSummary Method Information OK\n");
            DostoreAnnotationAssociation(selectsql,aasp->localID,aasp->targetliststr,aasp->relname,aasp->annotationContent);
        
            
            
                        
        }
    }
    SPI_finish();
    
    
}

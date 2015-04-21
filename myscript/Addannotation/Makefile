SOURCES=addannotation.c 
FSOURCES=addquery.c
TSOURCES=setPMode.c
SOBJS=$(SOURCES:.c=.o)
FOBJS=$(FSOURCES:.c=.o)
TOBJS=$(TSOURCES:.c=.o)
FSOBJS=$(FSOURCES:.c=.so)
TSOBJS=$(TSOURCES:.c=.so)
DS=$(SOURCES:.c=.h)
CC=cc
DEBUG=-c
CFLAGS= -I /Users/dongqingxiao/Documents/ResearchProject2014/pgsqlProject/include/server
BFLAGS=-bundle -flat_namespace -undefined suppress -o

all:p1 p2
p1: $(FSOURCES) $(SOBJS) $(TOBJS)
	$(CC) $(DEBUG) $(FSOURCES) $(CFLAGS) 
	$(CC) $(BFLAGS) $(FSOBJS)  $(FOBJS) $(SOBJS)
	$(CC) $(DEBUG) $(TSOURCES) $(CFLAGS)

p2: doextrawork.c addannotation.o $(TOBJS)
	$(CC) $(DEBUG) doextrawork.c $(CFLAGS)
	$(CC) $(BFLAGS) doextrawork.so doextrawork.o addannotation.o
	$(CC) $(BFLAGS) $(TSOBJS) $(TOBJS)
	
        
addannotation.o: addannotation.h addannotation.c 
	$(CC) $(DEBUG) $(CFLAGS) addannotation.c

    
clean:
	rm $(FOBJS) $(FSOBJS)
	rm $(SOBJS)
	rm $(TOBJS) $(TSOBJS)
	rm doextrawork.o 
	rm doextrawork.so


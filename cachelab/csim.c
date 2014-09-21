/*
 * NAME: Zechen Zhang
 * AndrewID: zechenz
 */

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <unistd.h>

typedef struct {
	unsigned long int flag; // 64-bit address with given s + b <= 10, so Flag would be longer than 32 bits  
	int valid;
	int lru;
	} Line;		 // a line in the sets

typedef struct {
	Line* line;
	int setStatus;	// count # of lines used in this set
	} Set;		 	// a set with E lines

typedef struct Trace {
	unsigned long int rawAddress;
	unsigned long int address;
	unsigned long int flag; 
	// Address stores the address read from traceFile, address contains the bits without offset and flag is only flagbits
	int setNumber;
	int size;
	char oper;
	} Trace;// Use this type of struct to process each line in trace file

FILE *traceFile = NULL;
int s, b, E;
int missCount = 0, hitCount = 0, evictionCount = 0;
int setMask;
unsigned long int flagMask;


void updateLRU(int lineNumber, int setNumber, Set* cache){ // update the line has number as lineNumber to the most recently used line and increase others' LRU value
    // LRU has a value range from 1 to E
	int LRUflag = cache[setNumber].line[lineNumber].lru; // use LRU flag to control the count of LRU. LRU value smaller than flag will ++.
	for (int i = 0; i < E; i++)
			if (cache[setNumber].line[i].lru < LRUflag) cache[setNumber].line[i].lru++;
	cache[setNumber].line[lineNumber].lru = 1;
}

int checkCache(Trace* trace, Set* cache) { // return 1 if hit

	trace->address = (trace->rawAddress >> b);
	trace->setNumber = trace->address & setMask;
	trace->flag = trace->address & flagMask;

	for (int i = 0; i < E; i++) 
			if ((cache[trace->setNumber].line[i].valid == 1) && (cache[trace->setNumber].line[i].flag == trace->flag)){ 
					updateLRU(i, trace->setNumber, cache);
					return 1;
			}
	return 0;
}

void updateCache(Trace* trace, Set* cache) { // will be called only if a miss happened
	int i = 0;
	if (cache[trace->setNumber].setStatus < E) { // the set is not full
		while (i < E) {
			if (cache[trace->setNumber].line[i].valid == 0) {
				cache[trace->setNumber].line[i].valid = 1;
				cache[trace->setNumber].line[i].flag = trace->flag;
				cache[trace->setNumber].line[i].lru = E;
				updateLRU(i, trace->setNumber, cache);
				break;
			}
			i++;
		}
		cache[trace->setNumber].setStatus++;
	}
	else // the set is full and eviction is required
		for (i = 0; i < E; i++)
			if (cache[trace->setNumber].line[i].lru == E) {
				cache[trace->setNumber].line[i].flag = trace->flag;
				updateLRU(i, trace->setNumber, cache);
				evictionCount++;
				break;
			}
}
 	
int cacheSimulator(){
	Set* cache;
	cache = (Set *)malloc((1 << s) * sizeof(Set));
	Trace trace;

	for (int i = 0; i < (1 << s); i++) {
		cache[i].line = (Line *)malloc(E * sizeof(Line));
		cache[i].setStatus = 0;
	}

	while (fscanf(traceFile,"%c %lx,%d", &trace.oper, &trace.rawAddress, &trace.size) != EOF) {
		switch(trace.oper) {
			case 'L':
			case 'S':
				if (checkCache(&trace, cache) == 1) {hitCount++;}
					else {
						missCount++;
						updateCache(&trace, cache);
					}
				break;
			case 'M':
				if (checkCache(&trace, cache) == 1) {hitCount += 2;}
					else {
						missCount++;
						updateCache(&trace, cache);
						hitCount++;
					}
				break;
			case 'I':
			default:
				break;
			} // switch case end
	} // while loop end
	return 0;
	for (int i = 0; i < s; i++)
		free(cache[i].line);
	free(cache);
}	

int main(int argc, char **argv){
	int hflag = 0, vflag = 0;
	char *argString = NULL;
	char *tracePath = NULL;

	int c;
	while ((c = getopt(argc, argv, "vhs:E:b:t:")) != -1){
		switch(c){
			case 'v':
				vflag = 1;
				break;
			case 'h':
				hflag = 1;
				break;
			case 's':
				argString = optarg;
				s = atoi(argString);
				break;
			case 'E':
				argString = optarg;
				E = atoi(argString);
				break;
			case 'b':
				argString = optarg;
				b = atoi(argString);
				break;
			case 't':
				tracePath = optarg;
				traceFile = fopen(tracePath, "r");
				break;
			case '?':
			default:
				printf("%s", "Error");
				return 1;
			}
	}

	setMask = ~ (-1 << s);
	flagMask =(unsigned long int) -1 << s;

	cacheSimulator();
	printSummary(hitCount, missCount, evictionCount);
	fclose(traceFile);
	return 0;
}

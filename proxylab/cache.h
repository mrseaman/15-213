#include "csapp.h"

#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

struct cacheobj 
{
	size_t size;
	struct cacheblock *prev,*next;
	char *uri,*buf;
};

typedef struct cacheobj obj; 

struct Cache
{
	size_t size;
	obj *head,*tail;
};

typedef struct Cache Cache;

void init_cache();
void cacheevit();
int cacheinsert(char*uri,size_t bufsize,char*buf);
obj *cachehit(char*uri);
void cacheclose();
void print_cache();

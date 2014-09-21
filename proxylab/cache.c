#include "cache.h"

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
	cacheblock *head,*tail;
};

typedef struct Cache Cache;

Cache cache;
pthread_rwlock_t cachelock;
pthread_rwlock_t listlock;


void init_cache()
{
	cache.head = NULL;
	cache.tail = NULL;
	cache.size = 0;

	pthread_rwlock_init(&cachelock, 0);
	pthread_rwlock_init(&listlock, 0);
	return;
}

void cacheevit(){
	if(cache.head == NULL){
		return;
	}
	else{	
		Free(cache.head->title);
		Free(cache.head->content);	
		cache.size -= (cache.head)->size;
		cache.head = (cache.head)->next;
		Free(cache.head->prev);
		(cache.head)->prev=NULL;
	}
	return;
}

int cacheinsert(char *uri,size_t cachecnt,char *cachebuf){
	pthread_rwlock_wrlock(&cachelock);
	pthread_rwlock_wrlock(&listlock);

	if(cachecnt == 0){
        pthread_rwlock_unlock(&listlock);
        pthread_rwlock_unlock(&cachelock);
		return 0;
	}

	if(cachecnt > MAX_OBJECT_SIZE){
        pthread_rwlock_unlock(&listlock);
        pthread_rwlock_unlock(&cachelock);

		return -1;
	}

	while(cache.size+size > MAX_CACHE_SIZE){
			cacheevit();
	}

	obj *newobj = malloc(sizeof(obj));
		newobj->size=size;
		newobj->uri = malloc(strlen(uri)+1);
		newobj->buf = malloc(size);
		memcpy(newobj->buf,buf,size);
		memcpy(newobj->uri,uri,strlen(uri));
		(newobj->uri)[strlen(uri)] = '\0';
		
		if(cache.size == 0){
			newobj->next = NULL;
			newobj->prev = NULL;
			cache.head = newobj;
			cache.tail = newobj;
			cache.size += newobj->size;
		}
		else{
			newobj->next = NULL;
			newobj->prev = cache.tail;
			(cache.tail)->next = newobj;
			cache.tail = newobj;
			cache.size += newobj->size;
		}
		pthread_rwlock_unlock(&listlock);
		pthread_rwlock_unlock(&cachelock);
		return 1;		
}
obj *cachehit(char *uri){
	obj*ptr = NULL;
	pthread_rwlock_rdlock(&cachelock);
	pthread_rwlock_wrlock(&listlock);

	ptr = cache.head;


	while(ptr != NULL){
		if(strcmp(ptr->uri,uri)==0)
		{
			if(ptr->prev != NULL && ptr->next != NULL)
			{
				(ptr->prev)->next=ptr->next;
				(ptr->next)->prev=ptr->prev;

				 ptr->next = NULL;
			     ptr->prev = cache.tail;
                 (cache.tail)->next = ptr;
                 cache.tail = ptr;
			}
			else if(ptr->prev != NULL && ptr->next == NULL)
			{
				
				(ptr->prev)->next=NULL;
				cache.tail=ptr->prev;
			}
			else if(ptr->prev == NULL && ptr->next != NULL)
			{
				(ptr->next)->prev=NULL;
				cache.head=ptr->next;

				ptr->next = NULL;
			    ptr->prev = cache.tail;
                (cache.tail)->next = ptr;
                cache.tail = ptr;
			}
			else
			{
				cache.head=NULL;
				cache.tail=NULL;
			}

			pthread_rwlock_unlock(&listlock);
			pthread_rwlock_unlock(&cachelock);

			return ptr;	     
		}
		ptr = ptr->next;
	}

    pthread_rwlock_unlock(&listlock);   
	pthread_rwlock_unlock(&cachelock);
	return NULL;
}
void cacheclose(){
	obj *ptr = cache.head;
	obj *tmp = NULL;	

	while(ptr != NULL) {
		tmp = ptr;
		ptr = ptr->next;
		Free(tmp->buf);
		Free(tmp->uri);
		Free(tmp);
	}

	pthread_rwlock_destroy(&cachelock);
	pthread_rwlock_destroy(&listlock);

	return;
}
void print_cache(){
	int objcnt = 0;
	obj *ptr = NULL;
    printf("***** Cache (size = %u) ******\n",(unsigned int)cache.size);
    
    ptr = cache.head;
    while (ptr != NULL){
       objcnt++;
       printf("p[%d]\t%s\n", objcnt, ptr->uri);
       ptr = ptr->next;
    }
    printf("*** end (object count = %d) ***\n", objcnt);
    return;
}
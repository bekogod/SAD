#include <glib.h>
#include <stdint.h>
#include <pthread.h>

#define KB(x) ((size_t)(x) * 1024)
#define MB(x) ((size_t)(x) * 1024 * 1024)
#define GB(x) ((size_t)(x) * 1024 * 1024 * 1024)

//struct defining the values of the Hashtable 
typedef struct filemeta {
	char *data;				// conteúdo do ficheiro
	size_t size;			// tamanho do conteúdo
} Indexmeta;

//structure containing the hashtable structure, global mutex and condition variable
typedef struct index {
	GHashTable *htable;
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	size_t current_cache_size;   // soma dos tamanhos dos ficheiros em cache
	size_t max_cache_size;       // limite máximo (definido por ti, ex: 100MB)
} Index;


//Initializes the index structure
//Returns NULL in case of failure and a pointer to the struct otherwise
Index* index_init();

//Adds a new key-value entry into the Hashtable
//Returns -1 in case of failure (or if the key already exists) and 0 otherwise
//Returns -2 in case of full cache
int index_add(Index *index, char* key, Indexmeta meta);

//Get the value (meta) for a specific key (key)
//Returns -1 in case of failure (or if the key does not exists) and 0 otherwise
int index_get(Index *index, char* key, Indexmeta *meta);

//Remove a key-value entry from the Hashtable 
//Returns -1 in case of failure (or if the key does not exists) and 0 otherwise
int index_remove(Index* index, char* key);

//Destroys the index structure
void index_destroy(Index* index);






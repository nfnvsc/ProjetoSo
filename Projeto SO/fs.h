#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include "lib/hash.h"
#include <pthread.h>

typedef struct tecnicofs_node {
    node* bstRoot;
    int nextINumber;
    pthread_mutex_t mutex_lock;
    pthread_rwlock_t rw_lock;
} tecnicofs_node;

typedef struct tecnicofs {
    tecnicofs_node** fs_nodes;
    int numberBuckets;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs, char* name);
tecnicofs* new_tecnicofs(int buckets);
tecnicofs_node* new_tecnicofs_node();
tecnicofs_node* get_node(tecnicofs* fs, char *name);

void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
void thread_fs_lock(tecnicofs_node* fs_node, int n);
void thread_fs_unlock(tecnicofs_node* fs_node);
void init_lock(tecnicofs_node* fs_node);

#endif /* FS_H */

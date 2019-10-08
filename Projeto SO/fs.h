#ifndef FS_H
#define FS_H
#include "lib/bst.h"
#include <pthread.h>



typedef struct tecnicofs {
    node* bstRoot;
    int nextINumber;
    pthread_mutex_t mutex_lock;
    pthread_rwlock_t rw_lock;
} tecnicofs;

int obtainNewInumber(tecnicofs* fs);
tecnicofs* new_tecnicofs();
void free_tecnicofs(tecnicofs* fs);
void create(tecnicofs* fs, char *name, int inumber);
void delete(tecnicofs* fs, char *name);
int lookup(tecnicofs* fs, char *name);
void print_tecnicofs_tree(FILE * fp, tecnicofs *fs);
void thread_fs_lock(tecnicofs* fs, int n);
void thread_fs_unlock(tecnicofs* fs);
void init_lock(tecnicofs* fs);

#endif /* FS_H */

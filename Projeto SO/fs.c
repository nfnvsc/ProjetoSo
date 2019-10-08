#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int obtainNewInumber(tecnicofs* fs) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(){
	tecnicofs* fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->bstRoot = NULL;
	fs->nextINumber = 0;
	init_lock(fs);
	return fs;
}

void init_lock(tecnicofs* fs){
	#ifdef MUTEX
	if (pthread_mutex_init(&fs->mutex_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
	#elif RWLOCK
	if (pthread_rwlock_init(&fs->rw_lock, NULL) != 0)
    {
        printf("\n rwlock init failed\n");
        exit(1);
    }
	#endif
}

void thread_fs_lock(tecnicofs* fs, int n){
	#ifdef MUTEX
	pthread_mutex_lock(&fs->mutex_lock);
	#elif RWLOCK
	if (n)	pthread_rwlock_wrlock(&fs->rw_lock);
	else pthread_rwlock_rdlock(&fs->rw_lock);
	#endif
}

void thread_fs_unlock(tecnicofs* fs){
	#ifdef MUTEX
	pthread_mutex_unlock(&fs->mutex_lock);
	#elif RWLOCK
	pthread_rwlock_unlock(&fs->rw_lock);
	#endif
}

void free_tecnicofs(tecnicofs* fs){
	free_tree(fs->bstRoot);
	free(fs);
}

void create(tecnicofs* fs, char *name, int inumber){
	thread_fs_lock(fs, 1);
	fs->bstRoot = insert(fs->bstRoot, name, inumber);
	thread_fs_unlock(fs);
}

void delete(tecnicofs* fs, char *name){
	thread_fs_lock(fs, 1);
	fs->bstRoot = remove_item(fs->bstRoot, name);
	thread_fs_unlock(fs);
}

int lookup(tecnicofs* fs, char *name){
	thread_fs_lock(fs, 0);
	node* searchNode = search(fs->bstRoot, name);
	thread_fs_unlock(fs);
	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	print_tree(fp, fs->bstRoot);
}

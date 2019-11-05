#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

pthread_mutex_t lock;


int obtainNewInumber(tecnicofs* fs, char* name) {
	int newInumber = ++(fs->nextINumber);
	return newInumber;
}

tecnicofs* new_tecnicofs(int numberBuckets){
	int i;
	tecnicofs* fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
		exit(EXIT_FAILURE);
	}
	fs->fs_nodes = malloc(sizeof(tecnicofs_node*)*numberBuckets);
	for(i = 0; i < numberBuckets; i++){
		fs->fs_nodes[i] = new_tecnicofs_node();
	}
	//pthread_rwlock_init(&fs->rw_lock, NULL);
	fs->numberBuckets = numberBuckets;
	fs->nextINumber = 0;

	return fs;

}

tecnicofs_node* new_tecnicofs_node(){
	tecnicofs_node* fs_node = malloc(sizeof(tecnicofs_node));
	if (!fs_node) {
		perror("failed to allocate tecnicofs_node");
		exit(EXIT_FAILURE);
	}
	fs_node->bstRoot = NULL;
	init_lock(fs_node);
	return fs_node;
}

void init_lock(tecnicofs_node* fs_node){
	#ifdef MUTEX
	if (pthread_mutex_init(&fs_node->mutex_lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        exit(1);
    }
	#elif RWLOCK
	if (pthread_rwlock_init(&fs_node->rw_lock, NULL) != 0)
    {
        printf("\n rwlock init failed\n");
        exit(1);
    }
	#endif
}

void thread_fs_lock(tecnicofs_node* fs_node, int n){
	#ifdef MUTEX
	if(pthread_mutex_lock(&fs_node->mutex_lock)) printf("Mutex lock error");;
	#elif RWLOCK
	if (n){
		if(pthread_rwlock_wrlock(&fs_node->rw_lock)) printf("RWLOCK lock error"); 
	}	
	else 
		if(pthread_rwlock_rdlock(&fs_node->rw_lock)) printf("RWLOCK lock error");
	#endif
}

void thread_fs_unlock(tecnicofs_node* fs_node){
	#ifdef MUTEX
	if (pthread_mutex_unlock(&fs_node->mutex_lock)) printf("MUTEX unlock error");
	#elif RWLOCK
	if (pthread_rwlock_unlock(&fs_node->rw_lock)) printf("RWLOCK unlock error");
	#endif
}

void free_tecnicofs(tecnicofs* fs){
	int i;
	tecnicofs_node* fs_node;
	for(i = 0; i<fs->numberBuckets; i++){
		fs_node = fs->fs_nodes[i];
		free_tree(fs_node->bstRoot);
		free(fs_node);
	}
	free(fs->fs_nodes);
	free(fs);
	
}

tecnicofs_node* get_node(tecnicofs* fs, char *name){
	int hash_number = hash(name, fs->numberBuckets);
	tecnicofs_node* node = fs->fs_nodes[hash_number]; 
	return node;

}

void create(tecnicofs* fs, char *name, int inumber){
	tecnicofs_node* fs_node = get_node(fs, name);

	thread_fs_lock(fs_node, 1);
	fs_node->bstRoot = insert(fs_node->bstRoot, name, inumber);
	thread_fs_unlock(fs_node);
}

void delete(tecnicofs* fs, char *name){
	tecnicofs_node* fs_node = get_node(fs, name);

	thread_fs_lock(fs_node, 1);
	fs_node->bstRoot = remove_item(fs_node->bstRoot, name);
	thread_fs_unlock(fs_node);
}

int lookup(tecnicofs* fs, char *name){
	tecnicofs_node* fs_node = get_node(fs, name);

	thread_fs_lock(fs_node, 0);
	node* searchNode = search(fs_node->bstRoot, name);
	thread_fs_unlock(fs_node);

	if ( searchNode ) return searchNode->inumber;
	return 0;
}

void rename(tecnicofs* fs, char* name, char* new_name){


}

void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	int i;
	tecnicofs_node* fs_node;
	for(i = 0; i < fs->numberBuckets; i++){
		fs_node = fs->fs_nodes[i];
		print_tree(fp, fs_node->bstRoot);

	}
}

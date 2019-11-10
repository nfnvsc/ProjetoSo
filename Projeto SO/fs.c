#include "fs.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

//pthread_mutex_t lock;


int obtainNewInumber(tecnicofs* fs, char* name) {
	int newInumber = ++fs->nextINumber;
	return newInumber;
}

tecnicofs* new_tecnicofs(int numberBuckets){
	int i;
	tecnicofs* fs = malloc(sizeof(tecnicofs));
	if (!fs) {
		perror("failed to allocate tecnicofs");
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

	}
	fs_node->bstRoot = NULL;
	init_lock(fs_node);
	return fs_node;
}

void init_lock(tecnicofs_node* fs_node){
	#ifdef MUTEX
	if (pthread_mutex_init(&fs_node->mutex_lock, NULL) != 0)
    {
        perror("\n mutex init failed\n");
    }
	#elif RWLOCK
	if (pthread_rwlock_init(&fs_node->rw_lock, NULL) != 0)
    {
        perror("\n rwlock init failed\n");
    }
	#endif
}

void destroy_lock(tecnicofs_node* fs_node){
	#ifdef MUTEX
    	if (pthread_mutex_destroy(&fs_node->mutex_lock) != 0){
            perror("mutex_destroy failed\n");
        }
    #elif RWLOCK
        if (pthread_rwlock_destroy(&fs_node->rw_lock) != 0){
        	perror("mutex_destroy failed\n");
        }
    #endif
}

void thread_fs_lock(tecnicofs_node* fs_node, int n){
	#ifdef MUTEX
		if(pthread_mutex_lock(&fs_node->mutex_lock)){ 
			perror("Mutex lock error\n");
		}
	
	#elif RWLOCK
		if (n){
			if(pthread_rwlock_wrlock(&fs_node->rw_lock)){
				perror("RWLOCK lock error\n"); 
			}
		}	
		else{ 
			if(pthread_rwlock_rdlock(&fs_node->rw_lock)){
				perror("RWLOCK lock\n"); 
			}
		}
	#endif
}

//------------------------ADDED TRYLOCK FUNCTION---------------------

//return 1 if locked
int thread_fs_trylock(tecnicofs_node* fs_node){
	#ifdef MUTEX
	if(pthread_mutex_trylock(&fs_node->mutex_lock)){
		//thread_fs_unlock(fs_node);
		return 0;
	}

	#elif RWLOCK
	if(pthread_rwlock_trywrlock(&fs_node->rw_lock)){
		//thread_fs_unlock(fs_node);
		return 0;
	}
	#endif
	return 1;
}

void thread_fs_unlock(tecnicofs_node* fs_node){
	#ifdef MUTEX
	if (pthread_mutex_unlock(&fs_node->mutex_lock)) perror("MUTEX unlock error\n");
	#elif RWLOCK
	if (pthread_rwlock_unlock(&fs_node->rw_lock)) perror("RWLOCK unlock error\n");
	#endif
}


void free_tecnicofs(tecnicofs* fs){
	int i;
	tecnicofs_node* fs_node;
	for(i = 0; i<fs->numberBuckets; i++){
		fs_node = fs->fs_nodes[i];
		destroy_lock(fs_node);
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

int tryLockBoth(tecnicofs_node* node1, tecnicofs_node* node2){
	int lock1, lock2;

	lock1 = thread_fs_trylock(node1);

	if (node1 != node2){
		lock2 = thread_fs_trylock(node2);
		if (lock1 && lock2) return 1;
		else{
			if (lock1) thread_fs_unlock(node1);
			if (lock2) thread_fs_unlock(node2);
			return 0;
		} 	
	}
	else return lock1;

}

//-------------------ADDED RENAME FUCNTION-------------------------
void renameFile(tecnicofs *fs, char* name, char* new_name){
	int ableToRename = 0;
	int numberAttempts = 0;
	tecnicofs_node* node_name = get_node(fs, name);
	tecnicofs_node* node_newName = get_node(fs, new_name);

	while(!tryLockBoth(node_name, node_newName)){
		usleep(numberAttempts * 1000);
		numberAttempts++;

	/*
		if (node_name == node_newName){
			#ifdef MUTEX
			if (!pthread_mutex_trylock(&node_name->mutex_lock))
				ableToRename = 1;
			else numberAttempts++;
			#elif RWLOCK
			if (!pthread_rwlock_trywrlock(&node_name->rw_lock))
				ableToRename = 1;
			else numberAttempts++;
			#else
			ableToRename = 1;
			#endif	
		}
		else{
			#ifdef MUTEX
			if (!pthread_mutex_trylock(&node_name->mutex_lock))
				if (!pthread_mutex_trylock(&node_newName->mutex_lock))
					ableToRename = 1;
				else{
					thread_fs_unlock(node_name);
					numberAttempts++;
				}
			else numberAttempts++;
			#elif RWLOCK
			if (!pthread_rwlock_trywrlock(&node_name->rw_lock))
				if (!pthread_rwlock_trywrlock(&node_newName->rw_lock))
					ableToRename = 1;
				else{
					thread_fs_unlock(node_name);
					numberAttempts++;
				}
			else numberAttempts++;
			#else
			ableToRename = 1;
			#endif
		}
		*/
	}
	
	node* searchNode = search(node_name->bstRoot, name);
	
	if (searchNode != NULL){
		int inumber = searchNode->inumber;
		if (search(node_newName->bstRoot, new_name) == NULL){
			printf("renaming\n");
			node_name->bstRoot = remove_item(node_name->bstRoot, name);
			node_newName->bstRoot = insert(node_newName->bstRoot, new_name, inumber);
		}
		else printf("%s already exists\n", new_name);
	}
	else printf("%s file to rename not found\n", name);
	
	if (node_name == node_newName) thread_fs_unlock(node_name);
	else{	
		thread_fs_unlock(node_name);
		thread_fs_unlock(node_newName);
	}
}



void print_tecnicofs_tree(FILE * fp, tecnicofs *fs){
	int i;
	tecnicofs_node* fs_node;
	for(i = 0; i < fs->numberBuckets; i++){
		fs_node = fs->fs_nodes[i];
		print_tree(fp, fs_node->bstRoot);

	}
}

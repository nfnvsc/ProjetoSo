#include "inodes.h"

#define MAX_OPEN_FILES 5

typedef struct open_file {
    int inumber;
    int mode;
} open_file;

int open(open_file* open_file_table, int inumber, int mode);
int close_open_file(open_file* open_file_table, int fd);
open_file* open_file_table_init();
void open_file_table_destroy(open_file* open_file_table);
int get_free_index(open_file* open_file_table);
int lookup_open_file_table(open_file* open_file_table, int inumber);
int get_info(open_file* open_file_table, int fd, int *mode, int *inumber);






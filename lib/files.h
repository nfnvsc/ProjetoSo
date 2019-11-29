#include "inodes.h"

#define MAX_OPEN_FILES 5

typedef struct open_file {
    int inumber;
    int mode;
} open_file;

int open(open_file* open_file_table, int inumber, int mode);
int close_open_file(open_file* open_file_table, int fd);
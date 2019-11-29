#include "inodes.h"

#define MAX_OPEN_FILES 5
#define FREE_INODE NULL


typedef struct open_file {
    int inumber;
    int mode;
} open_file;

int open(open_file* open_file_table, int inumber, int mode);
int close(open_file* open_file_table, int fd);
int read(int fd, char *buffer, int len);
int write(int fd, char *buffer, int len);
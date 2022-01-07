#include <stdio.h>
//used by both sender and reciver for the purpose of threading
typedef struct {
    FILE *fp;
    int sockfd;
    int size;
    int thread_id;
    long offset;
} thread_args;

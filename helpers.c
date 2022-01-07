#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#define CHUNK_SIZE_BYTES 1024
#define BASE_PORT 8080
#define IP "127.0.0.1"



//recives an integer over the socket and returns 
//copied from stack overflow
int receive_int(int *num, int fd)
{
    int32_t ret;
    char *data = (char*)&ret;
    int left = sizeof(ret);
    int rc;
    do {
        rc = read(fd, data, left);
        if (rc <= 0) { 
          perror("[-] Error writing int to socket");
            return -1;
        }
        else {
            data += rc;
            left -= rc;
        }
    }
    while (left > 0);
    *num = ntohl(ret);
    return 0;
}


//sends an integer over the socket to the receiver
//copied from stackoverflow
int send_int(int num, int fd)
{
    int32_t conv = htonl(num);
    char *data = (char*)&conv;
    int left = sizeof(conv);
    int rc;
    do {
        rc = write(fd, data, left);
        if (rc < 0) {
            perror("[-] Error writing int to socket");
            return -1;
        }
        else {
            data += rc;
            left -= rc;
        }
    }
    while (left > 0);
    return 0;
}
//calculates the file size in bytes for given file pointer
int file_size_calculate(FILE *fp)
{
    fseek(fp, 0L, SEEK_END);
    int size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    return size;
}

//port for ith thread
int port_for_ith_thread(int i)
{
    return BASE_PORT+2*i;
}
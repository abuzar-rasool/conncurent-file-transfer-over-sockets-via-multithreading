#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include "thread_data.c"
#include "helpers.c"
int no_of_threads;
char *file_name;
//function that is responsible for sending file in each thread
void *thread(void *args)
{
  FILE *fp = ((thread_args *)args)->fp;
  int sockfd = ((thread_args *)args)->sockfd;
  int thread_id = ((thread_args *)args)->thread_id;
  int n;
  char buffer[CHUNK_SIZE_BYTES];
  int total_in_bytes = 0;
  //recive offset_buffer from client
  char *offset_buffer = (char *)malloc(sizeof(char) * CHUNK_SIZE_BYTES);
  bzero(offset_buffer, CHUNK_SIZE_BYTES);
  read(sockfd, offset_buffer, CHUNK_SIZE_BYTES);
  //convert offset_buffer to long
  long base_offset = atol(offset_buffer);
  printf("[+]Thread %d recived offset %ld at port %d\n", thread_id, base_offset, port_for_ith_thread(thread_id));
  while (1)
  {
    n = read(sockfd, buffer, CHUNK_SIZE_BYTES);
    if (n <= 0)
    {
      break;
    }
    total_in_bytes += n;
    pwrite(fileno(fp), buffer, n, base_offset);
    base_offset += n;
    bzero(buffer, CHUNK_SIZE_BYTES);
  }
  printf("[+]Total bytes read on port %d in thread %d: %d\n", port_for_ith_thread(thread_id), thread_id, total_in_bytes);
  free(offset_buffer);
  return NULL;
}
//parent function creating the file pointer and threads
void recv_file(int *sockfd)
{
  //adding prefix to file name recvd_
  char *file_prefix = "recvd_";
  char *file_name_with_prefix = (char *)malloc(sizeof(char) * (strlen(file_prefix) + strlen(file_name)));
  strcpy(file_name_with_prefix, file_prefix);
  strcat(file_name_with_prefix, file_name);
  FILE *fp = fopen(file_name_with_prefix, "w");
  int i;
  pthread_t thread_ids[no_of_threads];
  for (int i = 0; i < no_of_threads; i++)
  {
    thread_args *args = malloc(sizeof(thread_args));
    args->sockfd = sockfd[i];
    args->thread_id = i;
    args->offset = 0;
    args->size = 0;
    args->fp = fp;
    pthread_create(&thread_ids[i], NULL, thread, args);
  }
  for (int i = 0; i < no_of_threads; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }
  free(file_name_with_prefix);
  fclose(fp);
  return;
}

//function that is responsible for establishing connection with the port
int establish_connection(int port)
{
  int sockfd;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("[-] Error creating socket");
    exit(1);
  }
  struct sockaddr_in server_addr, new_addr;
  socklen_t addr_size;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(IP);
  if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("[-] Error binding");
    exit(1);
  }
  printf("[+]Connecting to the port %d\n", port);
  if (listen(sockfd, 10) < 0)
  {
    perror("[-]Error in listening");
    exit(1);
  }

  addr_size = sizeof(new_addr);
  int new_sock = accept(sockfd, (struct sockaddr *)&new_addr, &addr_size);
  if (new_sock < 0)
  {

    perror("[-]Error in accepting");
    exit(1);
  }
  printf("[+]Connection established with port %d\n", port);
  close(sockfd);
  return new_sock;
}

int recieve_thread_qty(int *no_of_threads)
{
  int sockfd = establish_connection(BASE_PORT);
  receive_int(no_of_threads, sockfd);
  return sockfd;
}

int main()
{
  int socfd_first = recieve_thread_qty(&no_of_threads);
  printf("[+]Received no_of_threads: %d\n", no_of_threads);
  //receive file name
  file_name = (char *)malloc(sizeof(char) * 20);
  read(socfd_first, file_name, 20);
  printf("[+]Received file name: %s\n", file_name);
  //recive file size
  int sockfd[no_of_threads];
  sockfd[0] = socfd_first;
  for (int i = 1; i < no_of_threads; i++)
  {
    sockfd[i] = establish_connection(port_for_ith_thread(i));
  }
  // file wirtten with prefix recvd_
  recv_file(sockfd);
  printf("[+]Data written in the file successfully.\n");
  //closing all the connections
  for (int i = 0; i < no_of_threads; i++)
  {
    close(sockfd[i]);
    printf("[+]Closed connection to port %d\n", port_for_ith_thread(i));
  }
  free(file_name);
  return 0;
}
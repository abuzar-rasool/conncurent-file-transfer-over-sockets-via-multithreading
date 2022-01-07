#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "thread_data.c"
#include "helpers.c"
//function that breaks file in chunks of multiples of size SIZE
//The last chunk may be smaller or grater than SIZE due to the fact
//that the last chunk may not be a multiple of SIZE
// If the file size is less than SIZE than only the file is only
// broken into one chunk which means that the file is sent in one thread
int no_of_threads;
char *file_name;
int *split_file(int n)
{
  int *arr = (int *)malloc(sizeof(int) * no_of_threads);
  if (n > CHUNK_SIZE_BYTES)
  {

    // initialize all values of arr to n/NO_OF_THREADS-(n/NO_OF_THREADS)%SIZE
    // the last value of arr should by n-(sum of all previous values in arr)
    int total_bytes_covered = 0;
    for (int i = 0; i < no_of_threads; i++)
    {
      if (i == no_of_threads - 1)
      {
        arr[i] = n - total_bytes_covered;
        break;
      }
      arr[i] = (n / no_of_threads);
      arr[i] = arr[i] - (n / no_of_threads) % CHUNK_SIZE_BYTES;
      total_bytes_covered += arr[i];
    }
    return arr;
  }
  else
  {
    for (int i = 0; i < no_of_threads; i++)
    {
      arr[i] = 0;
    }
    arr[0] = n;
    return arr;
  }
}

void *thread(void *args)
{
  int size_in_bytes_to_send = ((thread_args *)args)->size;
  int sockfd = ((thread_args *)args)->sockfd;
  FILE *file = ((thread_args *)args)->fp;
  int thread_id = ((thread_args *)args)->thread_id;
  long base_offset = ((thread_args *)args)->offset;
  int n;
  char buffer[CHUNK_SIZE_BYTES];
  //convert base_offset to char buffer'
  char *offset_buffer = (char *)malloc(sizeof(char) * CHUNK_SIZE_BYTES);
  sprintf(offset_buffer, "%ld", base_offset);
  //send offset_buffer to server
  send(sockfd, offset_buffer, CHUNK_SIZE_BYTES, 0);
  printf("[+]Thread %d sent offset %ld on port %d\n", thread_id, base_offset, port_for_ith_thread(thread_id));
  int bytes_sent = 0;
  while (1)
  {
    bzero(buffer, CHUNK_SIZE_BYTES);
    if ((bytes_sent >= (size_in_bytes_to_send)))
    {
      break;
    }
    n = pread(fileno(file), buffer, CHUNK_SIZE_BYTES, base_offset + bytes_sent);
    if (n <= 0)
    {
      break;
    }
    write(sockfd, buffer, n);
    bytes_sent += n;
  }
  printf("[+]Total bytes sent by thread %d on port %d: %d\n", thread_id, port_for_ith_thread(thread_id), bytes_sent);
  free(offset_buffer);
  return NULL;
}

long *get_offsets(int *i)
{
  long *offsets = (long *)malloc(sizeof(long) * no_of_threads);
  offsets[0] = 0;
  for (int j = 1; j < no_of_threads; j++)
  {
    offsets[j] = offsets[j - 1] + i[j - 1];
  }
  return offsets;
}

void send_file(int *sockfd)
{
  //send file in threads
  pthread_t thread_ids[no_of_threads];
  FILE *fp = fopen(file_name, "r");
  int file_size = file_size_calculate(fp);
  printf("[+]File size: %d\n", file_size);
  int *arr = split_file(file_size);
  long *offsets = get_offsets(arr);
  //print total sum of arr
  int sum = 0;
  for (int i = 0; i < no_of_threads; i++)
  {
    sum += arr[i];
  }
  printf("[+]Total bytes to send: %d\n", sum);
  for (int i = 0; i < no_of_threads; i++)
  {
    printf("[+]Bytes to send in thread %d at port %d: %d\n", i, port_for_ith_thread(i), arr[i]);
    thread_args *args = (thread_args *)malloc(sizeof(thread_args));
    args->sockfd = sockfd[i];
    args->fp = fp;
    args->thread_id = i;
    args->size = arr[i];
    args->offset = offsets[i];
    pthread_create(&thread_ids[i], NULL, thread, (void *)args);
  }
  for (int i = 0; i < no_of_threads; i++)
  {
    pthread_join(thread_ids[i], NULL);
  }
  free(arr);
  free(offsets);
  fclose(fp);
  return;
}

int establish_connection(int port)
{
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
  {
    perror("[-] Error creating socket");
    exit(1);
  }
  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = port;
  server_addr.sin_addr.s_addr = inet_addr(IP);
  if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    perror("[-]Error in connection");
    exit(1);
  }
  printf("[+]Connected to port %d\n", port);
  sleep(1);
  return sockfd;
}

int send_no_of_threads()
{
  int sockfd = establish_connection(BASE_PORT);
  send_int(no_of_threads, sockfd);
  sleep(1);
  return sockfd;
}

int main()
{
  //take input number of threads from the user
  printf("Enter number of threads: ");
  scanf("%d", &no_of_threads);
  //take the input from the user for option of file
  //1. send.pdf
  //2. send.mp4
  //3. send.txt
  //4. send.png
  int file_input;
  printf("Enter file to send: \n1. pdf_file.pdf\n2. video.mp4\n3. image.png\n4. zip_file.zip\n");
  scanf("%d", &file_input);
  file_name = (char *)malloc(sizeof(char) * 20);
  if (file_input == 1)
  {
    file_name = "pdf_file.pdf";
  }
  else if (file_input == 2)
  {
    file_name = "video.mp4";
  }
  else if (file_input == 3)
  {
    file_name = "image.png";
  }
  else if (file_input == 4)
  {
    file_name = "zip_file.zip";
  }
  printf("[+]File name: %s\n", file_name);
  int sockfd[no_of_threads];
  sockfd[0] = send_no_of_threads();
  //send file name to server via sockfd[0]
  write(sockfd[0], file_name, strlen(file_name));
  sleep(1);
  for (int i = 1; i < no_of_threads; i++)
  {
    sockfd[i] = establish_connection(port_for_ith_thread(i));
  }
  send_file(sockfd);

  printf("[+]File data sent successfully.\n");
  printf("[+]Closing the connection.\n");
  for (int i = 0; i < no_of_threads; i++)
  {
    close(sockfd[i]);
  }
  return 0;
}

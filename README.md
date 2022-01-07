# Conncurent File Transfer Over Sockets Via Multithreading

## sender.c
The sender asks the user to input the number of the files and then displays the follwoing menu to select the files.
Enter file to send: 
1. pdf_file.pdf
2. video.mp4
3. image.png
4. zip_file.zip


```plaintext
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

```

The information for of threads and the file name is then transfered over the socket 8080 via TCP vie send_no_of_threads() function in the main thread. While the socket connection is still open the program continues to make connections via establish_connection(port).
Let's say if i have n threads so the program would make connections from 8080 + 2*n with a gap of 2.

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


get_offsets() creates the offset at which the file is to be writted by pread()


```plaintext
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
```

The send_file() function is then called which transfers the file over socket identified by sockfd's by breaking them in threads and calling the thread() function. pread is used to read the information on specific offset


```plaintext
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

```

## reciver.c
The revier recives the file over the ports opened by the sender. The ports are same as open by the sender and files are witten by each thread of sender on a specific port. From each port first the offset info is recived and then the file is written.




```plaintext

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
```

establish_connection(int port) establishes a connection to a particular port after the thread info is recived via recive_thread_qty().


```plaintext

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

```

The recv_file() function take multiple socket dispcriptors and recives data and thiere offset info over each port in a different thread, by using thread() function.
The file is written simoltaneously


```plaintext
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
```

## GENERAL NOTES
Use make all  : to make generate all code
Use make clean : to clean previous codes
make sender and make reciver to make the reciver and sender binary files

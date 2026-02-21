//
// client.c: A very, very primitive HTTP client.
// 
// To run, try: 
//      client hostname portnumber filename
//
// Sends one HTTP request to the specified HTTP server.
// Prints out the HTTP response.
//
// For testing your server, you will want to modify this client.  
// For example:
// You may want to make this multi-threaded so that you can 
// send many requests simultaneously to the server.
//
// You may also want to be able to request different URIs; 
// you may want to get more URIs from the command line 
// or read the list from a file. 
//
// When we test your server, we will be using modifications to this client.
//

#include "io_helper.h"
#include <pthread.h>

#define MAXBUF (8192)
#define NUMREQ 100

//
// Send an HTTP request for the specified file 
//
void client_send(int fd, char *filename) {
    char buf[MAXBUF];
    char hostname[MAXBUF];
    
    gethostname_or_die(hostname, MAXBUF);
    
    /* Form and send the HTTP request */
    sprintf(buf, "GET %s HTTP/1.1\n", filename);
    sprintf(buf, "%shost: %s\n\r\n", buf, hostname);
    write_or_die(fd, buf, strlen(buf));
}

//
// Read the HTTP response and print it out
//
FILE* fp;

void client_print(int fd) {
    char buf[MAXBUF];  
    int n;
    
    // Read and display the HTTP Header 
    n = readline_or_die(fd, buf, MAXBUF);
    while (strcmp(buf, "\r\n") && (n > 0)) {
	fprintf(fp, "Header: %s", buf);
	n = readline_or_die(fd, buf, MAXBUF);
	
	// If you want to look for certain HTTP tags... 
	// int length = 0;
	//if (sscanf(buf, "Content-Length: %d ", &length) == 1) {
	//    printf("Length = %d\n", length);
	//}
    }
    
    // Read and display the HTTP Body 
    n = readline_or_die(fd, buf, MAXBUF);
    while (n > 0) {
	fprintf(fp, "%s", buf);
	n = readline_or_die(fd, buf, MAXBUF);
    }
}

typedef struct {
    char* host, *filename;
    int port;
} thread_args;


void* threads_send(void* arg){
    thread_args* args = (thread_args*)arg;
    int clientfd = open_client_fd_or_die(args->host, args->port);
    client_send(clientfd, args->filename);
    client_print(clientfd);
    close_or_die(clientfd);
    return 0;
}

int main(int argc, char *argv[]) {
    
    if (argc != 4) {
	fprintf(stderr, "Usage: %s <host> <port> <filename>\n", argv[0]);
	exit(1);
    }

    fp = fopen("msg.txt", "w");

    thread_args* args = malloc(sizeof(thread_args));
    args->host = argv[1];
    args->port = atoi(argv[2]);
    args->filename = argv[3];
    
    /* Open a single connection to the specified host and port */
    // clientfd = open_client_fd_or_die(host, port);
    
    // client_send(clientfd, filename);
    // client_print(clientfd);
    
    // close_or_die(clientfd);

    pthread_t *threads = malloc(NUMREQ * sizeof(pthread_t));
    if (!threads) {
        perror("malloc\n");
        exit(1);
    }


    for (int i = 0; i < NUMREQ; i++){
        int rc = pthread_create(&threads[i], NULL, threads_send, (void *)args);
        if (rc != 0){
            fprintf(stderr, "pthread_create failed at %d\n", i);
            for (int j = 0; j < i; j++){
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(args);
            fclose(fp);
            exit(1);
        }
    }
    for (int i = 0; i < 100; i++){
        int rc = pthread_join(threads[i], NULL);
        if (rc != 0){
            perror("pthread_join failed\n");

        }
    }
    free(args);
    fclose(fp);
    free(threads);
    exit(0);
}

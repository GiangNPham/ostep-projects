#include <stdio.h>
#include "request.h"
#include "io_helper.h"
#include <bits/getopt_core.h>
#include <pthread.h>

pthread_cond_t has_work = PTHREAD_COND_INITIALIZER; // same with using pthread_cond_init()
pthread_cond_t available_buffer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock_buffer = PTHREAD_MUTEX_INITIALIZER;

int num_work = 0;
int start = 0, end = 0;
int buffer_size = 1;


char default_root[] = ".";

void* handling_request(void* arg){
	int* buffer = (int*) arg;
	while (1){
		pthread_mutex_lock(&lock_buffer);

		while (num_work == 0){
			pthread_cond_wait(&has_work, &lock_buffer);
		}

		int connfd = buffer[start];
		start = (start+1)%buffer_size;
		num_work--;
		pthread_cond_signal(&available_buffer);
		pthread_mutex_unlock(&lock_buffer);

		printf("Thread %x is handling request:\n", (unsigned long)pthread_self());
		request_handle(connfd);
		close_or_die(connfd);
	}

}

//
// ./wserver [-d <basedir>] [-p <portnum>] 
// 
int main(int argc, char *argv[]) {
    int c;
    char *root_dir = default_root;
    int port = 10000;
	int threads = 1;
	char* alg = "FIFO"; // static data (not on heap or stack)
    
    while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1)
	switch (c) {
	case 'd':
	    root_dir = optarg;
	    break;
	case 'p':
	    port = atoi(optarg);
	    break;
	case 't':
		threads = atoi(optarg);
		break;
	case 'b':
		buffer_size = atoi(optarg);
		break;
	case 's':
		alg = optarg;
		break;
	default:
	    fprintf(stderr, "usage: wserver [-d basedir] [-p port] [-t threads] [-b buffer] [-s algo]\n");
	    exit(1);
	}

    // run out of this directory
    chdir_or_die(root_dir);

	// char cwd[256]; 
    // if (getcwd(cwd, sizeof(cwd)) != NULL) {
    //     printf("Current working directory: %s\n", cwd);
    // } else {
    //     perror("getcwd error");
    // }

	// create buffers to store requests
	int* buffer = malloc(buffer_size * sizeof(int));

	// create pool of workers
	pthread_t* workers = malloc(threads * sizeof(pthread_t));
	for (int i = 0; i < threads; i++){
		pthread_create(&workers[i], NULL, handling_request, buffer);
	}

    // now, get to work
    int listen_fd = open_listen_fd_or_die(port);
    while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		// request_handle(conn_fd);
		// close_or_die(conn_fd);

		// acquire the lock for the buffer
		// put into the buffer

		pthread_mutex_lock(&lock_buffer);
		while (num_work == buffer_size) { // wait for the buffer has empty slots
			pthread_cond_wait(&available_buffer, &lock_buffer);
		}

		buffer[end] = conn_fd;
		num_work++;
		end = (end+1)%buffer_size;
		pthread_cond_signal(&has_work);
		pthread_mutex_unlock(&lock_buffer);
    }
	
	free(buffer);
	free(workers);
    return 0;
}


    


 

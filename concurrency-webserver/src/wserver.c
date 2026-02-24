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

// test SFF by having only 1 thread on the server
void* handling_SFF_request(void* arg){
	conn_t* buffer = (conn_t*) arg;
	while (1){
		pthread_mutex_lock(&lock_buffer);

		while (num_work == 0){
			pthread_cond_wait(&has_work, &lock_buffer);
		}

		conn_t req = buffer[end-1]; // must copy to the thread's stack, because if use the shared buffer, after releasing the lock, master thread can overwrite that block
		end--;
		num_work--;
		pthread_cond_signal(&available_buffer);
		pthread_mutex_unlock(&lock_buffer);
		
		printf("Thread %x is handling request:\n", (unsigned long)pthread_self());
		request_handle_without_header(&req);
		printf("File sent: %s\n", req.filename);
		close_or_die(req.fd);
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
	if (!strcmp(alg, "FIFO")) {
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

	// SSF scheduling
	conn_t* buffer = malloc(buffer_size * sizeof(conn_t));

	pthread_t* workers = malloc(threads * sizeof(pthread_t));
	for (int i = 0; i < threads; i++){
		pthread_create(&workers[i], NULL, handling_SFF_request, buffer);
	}

	int listen_fd = open_listen_fd_or_die(port);
	while (1) {
		struct sockaddr_in client_addr;
		int client_len = sizeof(client_addr);
		int conn_fd = accept_or_die(listen_fd, (sockaddr_t *) &client_addr, (socklen_t *) &client_len);
		
		// read the header

		conn_t req;
		req.fd = conn_fd;
		
		readline_or_die(conn_fd, req.buf, MAXBUF);

    	sscanf(req.buf, "%s %s %s", req.method, req.uri, req.version);

		if (strcasecmp(req.method, "GET")) {
			request_error(conn_fd, req.method, "501", "Not Implemented", "server does not implement this method");
			continue;
		}

		req.is_static = request_parse_uri(req.uri, req.filename, req.cgiargs);
		if (stat(req.filename, &req.sbuf) < 0) {
			request_error(conn_fd, req.filename, "404", "Not found", "server could not find this file");
			continue; // master thread skips this request
		}

		// acquire the lock for the buffer
		// put into the buffer

		pthread_mutex_lock(&lock_buffer);
		while (num_work == buffer_size) { // wait for the buffer has empty slots
			pthread_cond_wait(&available_buffer, &lock_buffer);
		}

		int id = 0;

		if (num_work != 0) {
            for (int i = end - 1; i >= 0; i--) {
                if (buffer[i].sbuf.st_size > req.sbuf.st_size) {
                    id = i + 1;
                    break;
                }
            }
            
            for (int i = end; i > id; i--) {
                buffer[i] = buffer[i - 1];
            }
        }

		buffer[id] = req;
		end++;
		num_work++;

		pthread_cond_signal(&has_work);
		pthread_mutex_unlock(&lock_buffer);
	}

	free(buffer);
	free(workers);
	return 0;

}
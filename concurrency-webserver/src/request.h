#ifndef __REQUEST_H__

#include <sys/stat.h>
#define MAXBUF (8192)

typedef struct {
	int fd;
	off_t size;
    int is_static;
    struct stat sbuf;
    char buf[MAXBUF], method[MAXBUF], uri[MAXBUF], version[MAXBUF], filename[MAXBUF], cgiargs[MAXBUF];
} conn_t;

void request_error(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int request_parse_uri(char *uri, char *filename, char *cgiargs);
void request_handle_without_header(conn_t* req);
void request_handle(int fd);

#endif // __REQUEST_H__

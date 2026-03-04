#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define min(a, b) (((a) < (b)) ? (a) : (b))

char *map = NULL;

typedef struct {
  char *data;
  size_t size;
} ZippedString;

ZippedString *results = NULL;
size_t zipSize, filesize;

void *zip(void *arg) {
  int index = *(int *)arg;
  free(arg);

  size_t start = (size_t)index * zipSize;
  size_t end_idx = filesize < (size_t)(index + 1) * zipSize
                       ? filesize
                       : (size_t)(index + 1) * zipSize;

  char *zipped = malloc(zipSize * 5);
  if (!zipped)
    return NULL; // Important: gracefully handle failed allocations

  size_t id = 0; // bytes, not index

  if (start < end_idx) {
    char prev = map[start];
    int cnt = 0;

    for (size_t i = start; i < end_idx; i++) {
      char cur = map[i];

      if (cur != prev) {
        memcpy(zipped + id, &cnt, sizeof(int));
        id += sizeof(int);
        zipped[id++] = prev;
        cnt = 0;
        prev = cur;
      }
      cnt++;
    }

    if (cnt) {
      memcpy(zipped + id, &cnt, sizeof(int));
      id += sizeof(int);
      zipped[id++] = prev;
    }
  }

  if (id > 0) {
    results[index].data = malloc(id);
    if (results[index].data) {
      memcpy(results[index].data, zipped, id); // copy bit by bit
    }
  } else {
    results[index].data = NULL;
  }

  results[index].size = id;
  free(zipped);
  return NULL;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "pzip: file1 [file2 ...]\n");
    exit(1);
  }

  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("Unable to open file");
    exit(1);
  }

  struct stat st;
  // fstat is safer to use since we already have the open fd
  if (fstat(fd, &st) < 0) {
    perror("Unable to stat file");
    close(fd);
    exit(1);
  }
  filesize = st.st_size;

  if (filesize == 0) {
    close(fd);
    exit(0);
  }

  map = (char *)mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
  if (map == MAP_FAILED) {
    perror("mmap\n");
    close(fd);
    exit(1);
  }

  int freeResource = get_nprocs();
  zipSize = filesize / freeResource + 1;

  results = malloc(freeResource * sizeof(ZippedString));

  pthread_t *threads = malloc(freeResource * sizeof(pthread_t));
  for (int i = 0; i < freeResource; i++) {
    int *id = malloc(sizeof(int));
    *id = i;
    int rc = pthread_create(&threads[i], NULL, zip, (void *)id);

    // if there's an error, stop creating more threads
    // use existing ones
    if (rc != 0) {
      free(id); // Fix: free 'id' to prevent memory leak
      freeResource = i;
      break;
    }
  }

  // printf("Create %d threads\n", freeResource);

  for (int i = 0; i < freeResource; i++) {
    pthread_join(threads[i], NULL);
  }

  if (freeResource == 1) {
    fwrite(results[0].data, results[0].size, 1, stdout);
  } else if (freeResource > 1) {
    int cnt = 0;
    char x = 0;
    int has_prev = 0;

    for (int i = 0; i < freeResource; i++) {
      for (size_t j = 0; j < results[i].size; j += 5) {
        int value;
        memcpy(&value, &results[i].data[j], sizeof(int));
        char cur = results[i].data[j + 4];

        if (has_prev && cur != x) {
          fwrite(&cnt, sizeof(int), 1, stdout);
          fwrite(&x, 1, 1, stdout);
          cnt = value;
          x = cur;
        } else {
          cnt += value;
          x = cur;
          has_prev = 1;
        }
      }
    }

    if (has_prev) {
      fwrite(&cnt, sizeof(int), 1, stdout);
      fwrite(&x, 1, 1, stdout);
    }
  }

  for (int i = 0; i < freeResource; i++) {
    free(results[i].data);
  }

  free(threads);
  free(results);
  munmap(map, filesize);
  close(fd);
  exit(0);
}
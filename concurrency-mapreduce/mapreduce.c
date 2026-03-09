#include "mapreduce.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Node {
  char *key;
  char *value;
  struct Node *next;
} Node;

static Node **buckets;
static Node **bucket_headers;
static pthread_mutex_t *bucket_locks;

char **file_names;
int num_partitions, num_files;
Partitioner partitionFunc;

typedef struct {
  Mapper map_func;
  int id, num_mappers;
} MapperThreadArg;

typedef struct {
  Reducer reduce_func;
  int id;
} ReducerThreadArg;

void *map_setup(void *arg) {
  MapperThreadArg *args = (MapperThreadArg *)arg;

  for (int i = 1; i <= num_files; i++) {
    if (i % args->num_mappers != args->id)
      continue;

    // printf("%d handles %d\n", args->id, i);
    args->map_func(file_names[i]);
  }
  free(args);

  return NULL;
}

char *get_next(char *key, int partition_number) {
  Node *curr = bucket_headers[partition_number];
  if (curr == NULL || strcmp(curr->key, key) != 0) {
    return NULL;
  }
  char *value = curr->value;
  bucket_headers[partition_number] = curr->next;
  return value;
}

void *reduce_setup(void *arg) {
  ReducerThreadArg *args = (ReducerThreadArg *)arg;
  while (bucket_headers[args->id] != NULL) {
    char *key = bucket_headers[args->id]->key;
    args->reduce_func(key, get_next, args->id);
  }

  free(arg);
  return NULL;
}

void MR_Emit(char *key, char *value) {
  // store the key/value pairs
  unsigned long hashedKey = MR_DefaultHashPartition(key, num_partitions);
  pthread_mutex_lock(&bucket_locks[hashedKey]);

  if (buckets[hashedKey] == NULL) {
    Node *head = malloc(sizeof(Node));
    head->key = strdup(key);
    head->value = strdup(value);
    head->next = NULL;

    buckets[hashedKey] = head;

    pthread_mutex_unlock(&bucket_locks[hashedKey]);
    return;
  }

  Node *cur = buckets[hashedKey];
  Node *prev = NULL;

  while (cur != NULL) {
    int res = strcmp(key, cur->key);
    if (res <= 0) {
      break;
    }

    prev = cur;
    cur = cur->next;
  }

  Node *newNode = malloc(sizeof(Node));
  newNode->key = strdup(key);
  newNode->value = strdup(value);

  // insert at the beginning
  if (prev == NULL) {
    newNode->next = buckets[hashedKey];
    buckets[hashedKey] = newNode;

    pthread_mutex_unlock(&bucket_locks[hashedKey]);
    return;
  }

  prev->next = newNode;
  newNode->next = cur;
  pthread_mutex_unlock(&bucket_locks[hashedKey]);
}

void MR_Run(int argc, char *argv[], Mapper map, int num_mappers, Reducer reduce,
            int num_reducers, Partitioner partition) {
  num_files = argc - 1;
  num_partitions = num_reducers;
  partitionFunc = partition;
  file_names = argv;

  buckets = malloc(num_partitions * sizeof(Node *));
  if (buckets == NULL) {
    perror("Cannot malloc buckets\n");
    exit(1);
  }

  for (int i = 0; i < num_partitions; i++) {
    buckets[i] = NULL;
  }

  bucket_locks = malloc(num_partitions * sizeof(pthread_mutex_t));
  if (bucket_locks == NULL) {
    perror("Cannot malloc locks\n");
    free(buckets);
    exit(1);
  }

  for (int i = 0; i < num_partitions; i++) {
    pthread_mutex_init(&bucket_locks[i], NULL);
  }

  pthread_t *mappers = malloc(sizeof(pthread_t) * num_mappers);
  if (mappers == NULL) {
    perror("Cannot malloc mappers\n");
    free(buckets);
    free(bucket_locks);
    exit(1);
  }

  for (int i = 0; i < num_mappers; i++) {
    // TODO: figure out how to recycle the threads to handle many files
    // maybe using a pool of mappers then use conditional variable to know when
    // all of the mappers are full
    MapperThreadArg *arg = malloc(sizeof(MapperThreadArg));
    arg->id = i;
    arg->map_func = map;
    arg->num_mappers = num_mappers;

    pthread_create(&mappers[i], NULL, map_setup, (void *)arg);
  }

  for (int i = 0; i < num_mappers; i++) {
    pthread_join(mappers[i], NULL);
  }
  free(mappers);

  // for (Node *curr = buckets[0]; curr != NULL; curr = curr->next) {
  //   printf("%s %s\n", curr->key, curr->value);
  // }

  // create threads for reducer

  bucket_headers = malloc(sizeof(Node *) * num_partitions);
  for (int i = 0; i < num_partitions; i++) {
    bucket_headers[i] = buckets[i];
  }

  pthread_t *reducers = malloc(sizeof(pthread_t) * num_partitions);
  for (int i = 0; i < num_reducers; i++) {
    ReducerThreadArg *args = malloc(sizeof(ReducerThreadArg));
    args->id = i;
    args->reduce_func = reduce;
    pthread_create(&reducers[i], NULL, reduce_setup, (void *)args);
  }

  for (int i = 0; i < num_partitions; i++) {
    pthread_join(reducers[i], NULL);
  }
  free(reducers);

  // clean up memory
  // bucket_headers
  // bucket
  // locks;
  for (int i = 0; i < num_partitions; i++) {
    Node *cur = buckets[i], *prev = NULL;
    while (cur != NULL) {
      prev = cur;
      cur = cur->next;
      free(prev->key);
      free(prev->value);
      free(prev);
    }
    pthread_mutex_destroy(&bucket_locks[i]);
  }
  free(buckets);
  free(bucket_headers);
  free(bucket_locks);
}
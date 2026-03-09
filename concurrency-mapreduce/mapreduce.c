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

Node **buckets;
pthread_mutex_t *bucket_locks;

int num_partitions;
Partitioner partitionFunc;

unsigned long MR_DefaultHashPartition(char *key, int num_partitions) {
  unsigned long hash = 5381;
  int c;
  while ((c = *key++) != '\0')
    hash = hash * 33 + c;
  return hash % num_partitions;
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
  // int num_files = argc - 1;
  num_partitions = num_reducers;
  partitionFunc = partition;

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
    pthread_create(&mappers[i], NULL, (void *)map, (void *)argv[1]);
  }

  for (int i = 0; i < num_mappers; i++) {
    pthread_join(mappers[i], NULL);
  }
  free(mappers);

  for (Node *curr = buckets[0]; curr != NULL; curr = curr->next) {
    printf("%s %s\n", curr->key, curr->value);
  }

  // create threads for reducer
}
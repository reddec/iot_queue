#include "iot_queue.h"
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset);
ssize_t safe_pwrite(int fd, const void *buf, size_t count, off_t offset);
void iot_queue_store_cache(struct queue_t *app, struct iot_queue_index_t chunk);
int iot_queue_read_index_at(const struct queue_t *app, struct iot_queue_index_t *chunk, size_t fd_offset);

void iot_print_app(const struct queue_t *app) {
  printf("queue_fd: %i\n", app->data_fd);
  printf("index_fd: %i\n", app->index_fd);
  printf("data_position: %li\n", app->data_position);
  printf("index_position: %li\n", app->index_position);
  printf("num_elements: %li\n", app->num_elements);
  printf("cache_reserved_num: %li\n", app->cache_reserved_num);
  printf("cache_num: %li\n", app->cache_num);
}

void iot_print_index(const struct iot_queue_index_t *chunk) {
  printf("size: %li\n", chunk->size);
  printf("offset: %li\n", chunk->offset);
}

int iot_queue_append_str(struct queue_t *app, const char *buffer) {
  return iot_queue_append(app, buffer, strlen(buffer));
}

int iot_queue_append(struct queue_t *app, const void *buffer, size_t size) {
  if (size <= 0) {
    return -1;
  }
  int rc = safe_pwrite(app->data_fd, buffer, size, app->data_position);
  if (rc < 0 || rc < size) {
    return -2;
  }
  struct iot_queue_index_t chunk = {
      .size=size,
      .offset=app->data_position
  };

  if (safe_pwrite(app->index_fd, (void *) &chunk, INDEX_CHUNK_SIZE, app->index_position) < INDEX_CHUNK_SIZE) {
    return -3;
  }
  app->index_position +=  INDEX_CHUNK_SIZE;
  app->data_position += size;
  ++(app->num_elements);
  iot_queue_store_cache(app, chunk);
  return 0;
}

ssize_t iot_queue_read(const struct queue_t *app, void *dest, size_t dest_size, size_t index) {
  struct iot_queue_index_t chunk = {0};
  if (iot_queue_read_index(app, &chunk, index) < 0) {
    return -1;
  }
  size_t read_size = dest_size;
  if (read_size > chunk.size) {
    read_size = chunk.size;
  }
  return safe_pread(app->data_fd, dest, read_size, chunk.offset);
}

int iot_queue_read_index(const struct queue_t *app, struct iot_queue_index_t *info, size_t index) {
  if (index >= app->num_elements) {
    return -1;
  }
  size_t history_depth = app->num_elements - index;
  if (history_depth <= app->cache_reserved_num && history_depth <= app->num_elements) {
    // in cache
    size_t cache_idx = (app->cache_num - history_depth) % app->cache_reserved_num;
    *info = app->cache[cache_idx];
    return 0;
  }
  // read from FS
  size_t fd_offset = index * INDEX_CHUNK_SIZE;
  return iot_queue_read_index_at(app, info, fd_offset);
}

int iot_queue_read_index_at(const struct queue_t *app, struct iot_queue_index_t *chunk, size_t fd_offset) {
  int rc = safe_pread(app->index_fd, (void *) chunk, INDEX_CHUNK_SIZE, fd_offset);
  if (rc < INDEX_CHUNK_SIZE) {
    perror("read index chunk");
    return -1;
  }
  return 0;
}

int iot_queue_warm_cache(struct queue_t *app) {
  uint64_t num = app->num_elements;
  if (num > app->cache_reserved_num) {
    num = app->cache_reserved_num;
  }
  struct iot_queue_index_t chunk;
  for (size_t i = app->num_elements - num; i < app->num_elements; ++i) {
    if (iot_queue_read_index(app, &chunk, i) < 0) {
      return -1;
    }
    iot_queue_store_cache(app, chunk);
  }
  return 0;
}

ssize_t safe_pread(int fd, void *buf, size_t count, off_t offset) {
  size_t complete = 0;
  while (complete < count) {
    ssize_t rd = pread(fd, buf + complete, count - complete, offset + complete);
    if (rd <= 0)
      return rd;
    complete += rd;
  }
  return complete;
}

ssize_t safe_pwrite(int fd, const void *buf, size_t count, off_t offset) {
  size_t complete = 0;
  while (complete < count) {
    ssize_t rd = pwrite(fd, buf + complete, count - complete, offset + complete);
    if (rd <= 0)
      return rd;
    complete += rd;
  }
  return complete;
}

int iot_queue_open(struct queue_t *app,
                   struct iot_queue_index_t *cache,
                   size_t num_cache,
                   int index_fd,
                   int data_fd) {
  struct queue_t target = {0};
  target.index_fd = index_fd;
  target.data_fd = data_fd;

  struct stat index_fd_stat;
  if (fstat(index_fd, &index_fd_stat) < 0) {
    perror("stat index file");
    return -3;
  }
  struct stat data_fd_stat;
  if (fstat(index_fd, &data_fd_stat) < 0) {
    perror("stat data file");
    return -3;
  }
  target.num_elements = index_fd_stat.st_size / INDEX_CHUNK_SIZE;
  target.index_position = INDEX_CHUNK_SIZE * target.num_elements;

  // read last index chunk to detect last data position
  if (target.num_elements > 0) {
    size_t last_offset = target.index_position - INDEX_CHUNK_SIZE;
    struct iot_queue_index_t last_index;
    if (iot_queue_read_index_at(&target, &last_index, last_offset) < 0) {
      return -4;
    }
    target.data_position = last_index.offset + last_index.size;
  }

  target.cache = cache;
  target.cache_reserved_num = cache ? num_cache : 0;
  if (iot_queue_warm_cache(&target) < 0) {
    perror("warm cache");
    return -5;
  }

  *app = target;
  return 0;
}

void iot_queue_store_cache(struct queue_t *app, struct iot_queue_index_t chunk) {
  if (app->cache_reserved_num == 0) {
    return;
  }
  size_t index = (app->cache_num) % app->cache_reserved_num;
  app->cache[index] = chunk;
  ++(app->cache_num);
}
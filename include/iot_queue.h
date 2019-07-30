//
// Created by baryshnikov on 29/07/19.
//

#ifndef IOT_QUEUE_IOT_QUEUE_H
#define IOT_QUEUE_IOT_QUEUE_H
#include <stdint.h>
#include <stdio.h>

// Describes single index record (a chunk)
struct iot_queue_index_t {
  uint64_t offset; // offset in a data (messages) file of a payload
  uint64_t size;   // actual payload size
}__attribute__((packed));

enum iot_queue_consts {
  INDEX_CHUNK_SIZE = sizeof(struct iot_queue_index_t) // Size of one index chunk
};

/**
 * Describes queue instance. Queue doesn't handle open/close file descriptors.
 * Thread not safe.
 */
struct iot_queue_t {
  // data
  int data_fd;                 // file descriptor for messages
  uint64_t data_position;      // last committed position in a file
  size_t num_elements;         // number of committed messages
  // index
  int index_fd;                // file descriptor for indexes
  uint64_t index_position;     // last committed position in a index file
  // cache
  struct iot_queue_index_t *cache; // in-memory cache for last N indexes
  size_t cache_reserved_num;   // maximum number of cached indexes
  size_t cache_num;            // always-growing counter of cached indexes (index = num % reserved)
};

/***
 * Initialize queue and warms cache.
 * Changes value of app only after successful initialization.
 * All file descriptors  should be open at-least in Read-Write mode and
 * supports positional reading/writing (pread, pwrite).
 * Cache acts as cyclic queue.
 *
 * @param app queue to be initialized (everything inside will be replaced)
 * @param cache cache buffer (should leave at least same time as instance of queue); can be NULL
 * @param num_cache size of cache in items; can be 0
 * @param index_fd index file descriptor;
 * @param data_fd data (messages) file descriptor
 * @return 0 on success, negative on error
 */
int iot_queue_open(struct iot_queue_t *app,
                   struct iot_queue_index_t *cache,
                   size_t num_cache,
                   int index_fd,
                   int data_fd);

/**
 * Append data to the queue. Successful return means fully written data and index files.
 * If cache is enabled, then latest N appended index chunks included to the cache.
 * @param app initialized queue instance
 * @param buffer message to append
 * @param size size of message
 * @return 0 on success, negative on error
 */
int iot_queue_append(struct iot_queue_t *app, const void *buffer, size_t size);

/**
 * Append one string to the queue. Same as iot_queue_append, but invokes strlen inside
 * @param app initialized queue instance
 * @param buffer zero-terminated string to append
 * @return 0 on success, negative on error
 */
int iot_queue_append_str(struct iot_queue_t *app, const char *buffer);

/**
 * Read single index chunk (from cache if possible)
 * @param app initialized queue instance
 * @param info destination for index information
 * @param index number of chunk to read in range from 0 (oldest) to num_elements - 1 (newest)
 * @return 0 on success, negative on error
 */
int iot_queue_read_index(const struct iot_queue_t *app, struct iot_queue_index_t *info, size_t index);

/**
 * Read stored message. It's safe to provide destination buffer lower than original message.
 * The operation can return positive number of read bytes less than size
 * of original message only if destination buffer less than message size.
 * @param app initialized queue instance
 * @param dest destination buffer for message
 * @param dest_size maximum size of message
 * @param index number of message to read in range from 0 (oldest) to num_elements - 1 (newest)
 * @return size of on read bytes of message, negative on error
 */
ssize_t iot_queue_read(const struct iot_queue_t *app, void *dest, size_t dest_size, size_t index);

/**
 * Print to STDOUT information about queue
 */
void iot_print_app(const struct iot_queue_t *app);

/**
 * Print to STDOUT information about index chunk
 */
void iot_print_index(const struct iot_queue_index_t *chunk);

#endif //IOT_QUEUE_IOT_QUEUE_H

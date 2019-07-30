//
// Created by baryshnikov on 30/07/2019.
//

#include "iot_queue_partition.h"

int iot_queue_pool_active_queue(const struct iot_queue_pool *pool, struct iot_queue_t **queue) {
  if (!pool) {
    return -1;
  }
  if (pool->used_partitions == 0) {
    return -1;
  }
  *queue = &pool->partitions[pool->used_partitions - 1];
  return 0;
}

int iot_queue_pool_choose_queue(const struct iot_queue_pool *pool,
                                struct iot_queue_partition *partition,
                                size_t message_offset) {
  if (!pool) {
    return -1;//paranoia
  }
  size_t offset = 0;
  for (size_t partition_idx = 0; partition_idx < pool->used_partitions; ++partition_idx) {
    size_t upper_limit = offset + pool->partitions[partition_idx].num_elements;
    if (message_offset >= offset && message_offset < upper_limit) {
      partition->offset = message_offset - offset;
      partition->pool_offset = offset;
      partition->queue = &pool->partitions[partition_idx];
      return 0;
    }
    offset = upper_limit;
  }
  return -1;
}

int iot_queue_pool_attach_queue(struct iot_queue_pool *pool, struct iot_queue_t *queue) {
  if (!pool) {
    return -1;//paranoia
  }
  if (pool->used_partitions >= pool->total_partitions) {
    return -1;
  }
  pool->partitions[pool->used_partitions] = *queue;
  ++(pool->used_partitions);
  return 0;
}
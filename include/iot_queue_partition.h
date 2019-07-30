//
// Created by baryshnikov on 30/07/2019.
//

#ifndef IOT_QUEUE_IOT_QUEUE_PARTITION_H
#define IOT_QUEUE_IOT_QUEUE_PARTITION_H
#include "iot_queue.h"

/**s
 * Pool of available queues.
 */
struct iot_queue_pool {
  struct iot_queue_t *partitions; // should be ordered. last queue is active
  size_t used_partitions;
  size_t total_partitions;
};

struct iot_queue_partition {
  struct iot_queue_t *queue; // chosen queue
  size_t pool_offset;        // accumulated offset through all queues
  size_t offset;             // offset in the queue
};

/**
 * Choose destination queue from partitions for reading
 *
 * @param pool initialized pool of queue partitions
 * @param queue destination pointer for chosen queue
 * @param message_offset message offset
 * @return 0 on success, -1 if queue not found
 */
int iot_queue_pool_choose_queue(const struct iot_queue_pool *pool,
                                struct iot_queue_partition *partition,
                                size_t message_offset);

/**
 * Attach existent queue to the pool (if there are available slots).
 * Lower bound (from) will be used as a number of limits of previous active queue.
 *
 * @param pool initialized pool of queue partitions
 * @param queue  existent queue
 * @return 0 on success, -1 if no available slots
 */
int iot_queue_pool_attach_queue(struct iot_queue_pool *pool, struct iot_queue_t *queue);

/**
 * Find active queue available for writing
 *
 * @param pool initialized pool of queue partitions
 * @param queue destination queue
* @return 0 on success, -1 if no available queues (empty pool)
 */
int iot_queue_pool_active_queue(const struct iot_queue_pool *pool, struct iot_queue_t **queue);
#endif //IOT_QUEUE_IOT_QUEUE_PARTITION_H

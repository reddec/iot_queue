//
// Created by baryshnikov on 30/07/19.
//

#ifndef IOT_QUEUE_UTILS_H
#define IOT_QUEUE_UTILS_H
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <iot_queue.h>
#include <string.h>

struct iot_queue_index_t cache[8] = {0};

struct queue_t app;

void deinit();

int init() {
  int index_fd = open("index.bin", O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0755);
  if (index_fd < 0) {
    perror("open index");
    return -1;
  }

  int data_fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC | O_SYNC, 0755);
  if (data_fd < 0) {
    close(index_fd);
    perror("open data");
    return -1;
  }
  int rc = iot_queue_open(&app, cache, sizeof(cache) / sizeof(cache[0]), index_fd, data_fd);
  if (rc < 0) {
    close(index_fd);
    close(data_fd);
    perror("open queue");
    return -1;
  }
  atexit(deinit);
  return 0;
}

void deinit() {
  close(app.index_fd);
  close(app.data_fd);
}

#endif //IOT_QUEUE_UTILS_H

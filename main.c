#include "iot_queue.h"
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>

int main() {
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

  struct queue_t app;
  struct iot_queue_index_t cache[16] = {0};

  int rc = iot_queue_open(&app, cache, sizeof(cache) / sizeof(cache[0]), index_fd, data_fd);
  if (rc < 0) {
    close(index_fd);
    close(data_fd);
    perror("open queue");
    return -1;
  }

  size_t N = 128;
  char buffer[128] = {0};
  for (int i = 0; i < N; ++i) {
    sprintf(buffer, "%i", i);
    if (iot_queue_append_str(&app, buffer) < 0) {
      return -2;
    }
  }
  iot_print_app(&app);
  for (uint64_t i = 0; i < N; ++i) {
    memset(buffer, 0, sizeof(buffer));
    if (iot_queue_read(&app, buffer, sizeof(buffer), i) < 0) {
      return -2;
    }
    puts(buffer);
  }

  close(index_fd);
  close(data_fd);
  return 0;
}
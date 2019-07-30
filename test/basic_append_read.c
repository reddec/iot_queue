//
// Created by baryshnikov on 30/07/19.
//
#include <stdlib.h>
#include "utils.h"

int main() {

  if (init() < 0) {
    return -1;
  }

  if (iot_queue_append_str(&app, "hello world") < 0) {
    return -2;
  }
  char buffer[32] = {0};
  if (iot_queue_read(&app, buffer, sizeof(buffer), 0) < 0) {
    return -3;
  }
  if (strcmp("hello world", buffer) != 0) {
    fprintf(stderr, "non-equal saved and loaded values");
    return -4;
  }
  return 0;
}
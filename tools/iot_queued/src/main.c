//
// Created by baryshnikov on 30/07/2019.
//
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <zmq.h>
#include <memory.h>
#include <iot_queue.h>

#define PARSE_ULONG(opt, target) case opt: { \
long val = strtol(optarg, NULL, 10);\
if (errno == ERANGE || val < 0) {\
print_usage(argv);\
return -1;\
}\
(target) = (size_t) val;\
break;\
}\

struct config_t {
  size_t cache_size;
  size_t line_buffer;
  const char *index_filename;
  const char *data_filename;
  const char *rep_binding;
};

struct config_t default_config() {
  struct config_t config = {
      .cache_size = 65536,
      .line_buffer = 8192,
      .index_filename = "index.bin",
      .data_filename = "data.bin",
      .rep_binding = "tcp://*:9888"
  };
  return config;
}

void print_usage(char *const *argv) {
  struct config_t def = default_config();
  printf("%s [flags...]\n", argv[0]);
  printf("-C, --cache-size <uint>\t[default: %li] size of cached index chunks\n", def.cache_size);
  printf("-L, --line-size <uint>\t[default: %li] maximum size of line (incoming message)\n", def.line_buffer);
  printf("-i, --index-file <path>\t[default: %s] path to index file\n", def.index_filename);
  printf("-d, --data-file <path>\t[default: %s] path to data file\n", def.data_filename);
  printf("-b, --bind <zmq>\t[default: %s] URL to bind API (rep) socket\n", def.rep_binding);
  printf("-h, --help\tshow this help\n");
}

int parse_arguments(int argc, char *const *argv, struct config_t *config) {
  static struct option long_options[] = {
      {"cache-size", optional_argument, NULL, 'C'},
      {"line-size", optional_argument, NULL, 'L'},
      {"index-file", optional_argument, NULL, 'i'},
      {"data-file", optional_argument, NULL, 'd'},
      {"bind", optional_argument, NULL, 'b'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}
  };
  int ch;
  while ((ch = getopt_long(argc, argv, "C:L:i:d:p:h", long_options, NULL)) != -1) {
    // check to see if a single character or long option came through
    switch (ch) {
    PARSE_ULONG('C', config->cache_size)
    PARSE_ULONG('L', config->line_buffer)
    case 'i': {
      config->index_filename = optarg;
      break;
    }
    case 'd': {
      config->data_filename = optarg;
      break;
    }
    case 'b': {
      config->rep_binding = optarg;
      break;
    }
    case 'h': {
      print_usage(argv);
      return -1;
    }
    default: {
      print_usage(argv);
      return -1;
    }
    }
  }
  return 0;
}

struct app_t {
  int index_fd;
  int data_fd;
  void *zctx;
  struct iot_queue_t queue;
  struct iot_queue_index_t *cache;
  const struct config_t *config; // not allocated by app
};

int cmd_push(struct app_t *app, void *socket) {
  int more;
  size_t more_size = sizeof(more);
  int rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  if (rc < 0) {
    return -1;
  }
  while (more) {
    zmq_msg_t part;
    rc = zmq_msg_init(&part);
    if (rc < 0) {
      return -1;
    }
    rc = zmq_recvmsg(socket, &part, 0);
    if (rc < 0) {
      zmq_msg_close(&part);
      return -1;
    }

    if (iot_queue_append(&app->queue, zmq_msg_data(&part), zmq_msg_size(&part)) < 0) {
      zmq_msg_close(&part);
      return -1;
    }

    rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
    if (rc < 0) {
      zmq_msg_close(&part);
      return -1;
    }
    zmq_msg_close(&part);
  };
  zmq_send_const(socket, "OK", 2, 0);
  return 0;
}

int cmd_fetch(struct app_t *app, void *socket) {
  int more;
  size_t more_size = sizeof(more);

  // get message offset
  zmq_msg_t offset_msg;
  int rc = zmq_msg_init(&offset_msg);
  if (rc < 0) {
    return -1;
  }
  rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  if (rc < 0 || !more) {
    zmq_msg_close(&offset_msg);
    return -1;
  }
  rc = zmq_msg_recv(&offset_msg, socket, 0);
  if (rc < 0) {
    zmq_msg_close(&offset_msg);
    return -1;
  }

  uint64_t offset = strtoull((const char *) zmq_msg_data(&offset_msg), NULL, 10);
  if (errno == ERANGE) {
    zmq_msg_close(&offset_msg);
    return -3;
  }
  zmq_msg_close(&offset_msg);

  // get limit
  zmq_msg_t limit_msg;
  rc = zmq_msg_init(&limit_msg);
  if (rc < 0) {
    return -1;
  }
  rc = zmq_getsockopt(socket, ZMQ_RCVMORE, &more, &more_size);
  if (rc < 0 || !more) {
    zmq_msg_close(&limit_msg);
    return -1;
  }
  rc = zmq_msg_recv(&limit_msg, socket, 0);
  if (rc < 0) {
    zmq_msg_close(&limit_msg);
    return -1;
  }
  uint64_t limit = strtoull((const char *) zmq_msg_data(&limit_msg), NULL, 10);
  if (errno == ERANGE) {
    zmq_msg_close(&limit_msg);
    return -3;
  }
  zmq_msg_close(&limit_msg);

  uint64_t top = offset + limit;
  if (top > app->queue.num_elements) {
    top = app->queue.num_elements;
  }
  printf("FROM %li TO %li\n", offset, top);
  rc = zmq_send_const(socket, "OK", 2, top > offset ? ZMQ_SNDMORE : 0);
  if (rc < 0) {
    return -4;
  }

  for (uint64_t i = offset; i < top; ++i) {
    struct iot_queue_index_t info;
    if (iot_queue_read_index(&app->queue, &info, i) < 0) {
      return -1;
    }
    zmq_msg_t msg;
    if (zmq_msg_init_size(&msg, info.size) < 0) {
      return -1;
    }

    if (iot_queue_read(&app->queue, zmq_msg_data(&msg), info.size, i) < 0) {
      zmq_msg_close(&msg);
      return -1;
    }

    if (zmq_msg_send(&msg, socket, i == top - 1 ? 0 : ZMQ_SNDMORE) < 0) {
      zmq_msg_close(&msg);
      return -1;
    }
    zmq_msg_close(&msg);
  }
  return 0;
}

int init_server(struct app_t *app) {
  void *rep_socket = zmq_socket(app->zctx, ZMQ_REP);
  if (!rep_socket) {
    return -1;
  }

  if (zmq_bind(rep_socket, app->config->rep_binding) != 0) {
    zmq_close(rep_socket);
    return -2;
  }

  for (;;) {
    zmq_msg_t command;
    zmq_msg_init(&command);
    int rc = zmq_msg_recv(&command, rep_socket, 0);

    if (rc == -1 && zmq_errno() == EAGAIN) {
      zmq_msg_close(&command);
      continue;
    } else if (rc == -1) {
      fprintf(stderr, "recvmsg: %i\n", zmq_errno());
      break;
    }
    const char *cmd = (const char *) zmq_msg_data(&command);
    size_t cmd_size = zmq_msg_size(&command);
    if (strncmp("PUSH", cmd, cmd_size) == 0) {
      rc = cmd_push(app, rep_socket);
    } else if (strncmp("FETCH", cmd, cmd_size) == 0) {
      rc = cmd_fetch(app, rep_socket);
    } else {
      rc = -1;
    }
    zmq_msg_close(&command);
    if (rc < 0) {
      zmq_send_const(rep_socket, "FAIL", 4, 0);
    }
  }

  zmq_close(rep_socket);
  return 0;
}

int run(struct app_t *app) {
  int rc = iot_queue_open(&app->queue, app->cache, app->config->cache_size, app->index_fd, app->data_fd);
  if (rc < 0) {
    return -1;
  }
  iot_print_app(&app->queue);
  if (init_server(app) < 0) {
    return -1;
  }

  return 0;
}

void close_app(struct app_t *app) {
  if (app->zctx) {
    zmq_ctx_destroy(app->zctx);
  }
  if (app->index_fd >= 0) {
    close(app->index_fd);
  }
  if (app->data_fd >= 0) {
    close(app->data_fd);
  }
  if (app->cache) {
    free(app->cache);
  }
}

int main(int argc, char *const *argv) {
  struct config_t config = default_config();
  if (parse_arguments(argc, argv, &config) < 0) {
    return -1;
  }
  struct app_t app = {0};
  app.config = &config;
  app.index_fd = -1;
  app.data_fd = -1;

  app.zctx = zmq_ctx_new();
  if (!app.zctx) {
    perror("initialize zmq context");
    return -1;
  }
  app.cache = (struct iot_queue_index_t *) malloc(sizeof(struct iot_queue_index_t) * config.cache_size);
  if (!app.cache) {
    perror("allocate cache");
    close_app(&app);
    return -1;
  }

  app.index_fd = open(config.index_filename, O_RDWR | O_CREAT | O_SYNC, 0755);
  if (app.index_fd < 0) {
    perror("open index");
    close_app(&app);
    return -1;
  }
  app.data_fd = open(config.data_filename, O_RDWR | O_CREAT | O_SYNC, 0755);
  if (app.data_fd < 0) {
    perror("open data");
    close_app(&app);
    return -1;
  }

  int rc = run(&app);

  close_app(&app);
  return rc;
}
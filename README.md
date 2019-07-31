# IOT-queue

Simple, lightweight and easy-to-use set of libraries and tools to make queues.

Architecture design principles (ADP):

1. SHOULD be capable to handle extreme amount of messages (billions)
2. Single message always can fit into memory (it means that messages usually relatively small)
3. SHOULD be reliable: if procedure (append) successfully completes the data and the index already persisted
4. SHOULD be consistent and handle situations like:
   a. power off/system corruption between appends
   b. power off/system corruption during writing data
   c. power off/system corruption during writing index
5. SHOULD be fast enough but speed is not a main concern (expected performance should be about: 300-500 messages/second)
6. SHOULD be append-only queue. Truncating is optional and supposed to be an offline process (still not yet clean)
7. Fetch newest messages SHOULD be faster than fetch historical messages

Coding design principles (CDP):

1. No memory allocations. At all. All buffers should be stack based or provided by external code
2. No "destructors": all resource allocation should be done by external code (like file descriptors)
3. Zero (or positive in case of buffer operations) return code means success, negative - error
4. Should be easy-to-use interfaces "by default" with advanced options


## iot_queued

Simple daemon for single queue with ZMQ interface.

Runtime flags:

```
-C, --cache-size <uint> [default: 65536] size of cached index chunks
-L, --line-size <uint>  [default: 8192] maximum size of line (incoming message)
-i, --index-file <path> [default: index.bin] path to index file
-d, --data-file <path>  [default: data.bin] path to data file
-b, --bind <zmq>        [default: tcp://*:9888] URL to bind API (rep) socket
-h, --help      show this help
```

Requirements:

* zeromq
* iot_queue

### API

#### PUSH

Append multiple messages to the queue.

* **Input**: <PUSH:string>, [<MSG:bytes>, ... ]
* **Output**: <OK:string> | <FAIL:string>

#### FETCH

Fetch messages from the queue starting from `OFFSET` with `LIMIT`.

* **Input**: <FETCH:string>, <OFFSET:string>, <LIMIT:string>
* **Output**: <OK:string>, [<MSG:bytes>, ... ] | <FAIL:string>


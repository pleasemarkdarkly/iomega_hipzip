typedef struct _pipe {
    unsigned char* in_buf;              // only changed by producer
    int in_avail;                       // only changed by producer
    unsigned char* out_buf;             // only changed by consumer (init by producer)
    int out_size;                       // only changed by consumer (init by producer)
    const char* msg;                    // message from consumer
    void* priv;                         // handler's data
} _pipe_t;

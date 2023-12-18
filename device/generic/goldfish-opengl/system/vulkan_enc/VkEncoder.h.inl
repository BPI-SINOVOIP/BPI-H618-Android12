    void flush();
    void lock();
    void unlock();
    void incRef();
    bool decRef();
    uint32_t refCount = 1;
    #define POOL_CLEAR_INTERVAL 10
    uint32_t encodeCount = 0;
    uint32_t featureBits = 0;

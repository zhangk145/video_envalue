#ifndef UTILS_CBQUEUE_H
#define UTILS_CBQUEUE_H

#include <list>
#include <pthread.h>

using namespace std;

class Queue {
public:
    Queue();
    Queue(int id);
    ~Queue();
    void put(void *pkt);
    void put_back(void *pkt);
    void flush();
    void abort();
    void signal();
    bool empty();
    int peek(void **pkt, bool block);
    int peek_at(void **pkt, int pos);
    int get(void **pkt, bool block);
    int get_at(void **pkt, int pos);
    int size();
    void clear();
    typedef void (*FreePktFunc)(void *pkt, bool freeMem);
    inline void set_freepkt_cb(FreePktFunc callback) {
        _freepkt_cb = callback;
    }
protected:
    virtual int get_internal(void *pkt);
    virtual int put_internal(void *pkt);
    void free_pkt(void *pkt, bool freeMem);
    list<void *> _queue;
    bool _abort_request;
    bool _signal;
    pthread_mutex_t _mutex;
    pthread_cond_t _cond;
    FreePktFunc _freepkt_cb;
private:
    
};

#endif


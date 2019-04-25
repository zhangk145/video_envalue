
#include "Queue.h"
#include "CBError.h"

Queue::Queue()
    : _abort_request(false),
      _signal(false),
      _mutex(PTHREAD_MUTEX_INITIALIZER),
      _cond(PTHREAD_COND_INITIALIZER),
      _freepkt_cb(NULL) {
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    _queue.clear();
}

Queue::Queue(int id)
    : _abort_request(false),
      _signal(false),
      _mutex(PTHREAD_MUTEX_INITIALIZER),
      _cond(PTHREAD_COND_INITIALIZER),
      _freepkt_cb(NULL) {
    pthread_mutex_init(&_mutex, NULL);
    pthread_cond_init(&_cond, NULL);
    _queue.clear();
}

Queue::~Queue() {
    clear();
    pthread_mutex_destroy(&_mutex);
    pthread_cond_destroy(&_cond);
}

void Queue::put(void *pkt) {

    pthread_mutex_lock(&_mutex);
    _queue.push_back(pkt);
    this->put_internal(pkt);
    pthread_cond_signal&_cond);
    pthread_mutex_unlock(&_mutex);
    
}

void Queue::put_back(void *pkt) {
    pthread_mutex_lock(&_mutex);
    _queue.push_front(pkt);
    this->put_internal(pkt);
    pthread_cond_signal&_cond);
    pthread_mutex_unlock(&_mutex);
}

bool Queue::empty() {
    bool ret;
    pthread_mutex_lock(&_mutex);
    ret = _queue.empty();
    pthread_mutex_unlock(&_mutex);
    return ret;
}

void Queue::free_pkt(void *pkt, bool freeMem) {
    if (_freepkt_cb != NULL) {
        //AV_LOGD(LOG_TAG, "[%s:%d]===\n", __FUNCTION__, __LINE__);
        _freepkt_cb(pkt, freeMem);
    }
}

void Queue::flush() {
    void *pkt = NULL;

    pthread_mutex_lock(&_mutex);
    _abort_request = true;
    pthread_cond_signal&_cond);
    for (list<void *>::iterator iter = _queue.begin(); iter != _queue.end(); ++iter) {
        pkt = *iter;
        free_pkt(pkt, false);
    }
    _abort_request = false;
    pthread_mutex_unlock(&_mutex);
}

void Queue::clear() {
    void *pkt = NULL;
    
    pthread_mutex_lock(&_mutex);
    _abort_request = true;
    pthread_cond_signal&_cond);
    while (!_queue.empty()) {
        pkt = _queue.front();
        _queue.erase(_queue.begin());
        free_pkt(pkt, true);
    }
    _abort_request = false;
    pthread_mutex_unlock(&_mutex);
}

void Queue::abort() {
    pthread_mutex_lock(&_mutex);

    _abort_request = true;

    pthread_cond_signal&_cond);

    pthread_mutex_unlock(&_mutex);
}

void Queue::signal() {
    pthread_mutex_lock(&_mutex);
    _signal = true;
    pthread_cond_signal&_cond);
    pthread_mutex_unlock(&_mutex);
}

int Queue::peek_at(void **pkt, int pos) {
    int i = 0;
    int ret = CBERROR(ERR_INVAL);
    
    pthread_mutex_lock(&_mutex);
    if (_queue.empty()) {
        pthread_mutex_unlock(&_mutex);
        return CBERROR(ERR_INVAL);
    }
    for (list<void *>::iterator iter = _queue.begin(); iter != _queue.end(); ++iter) {
        if (pos == i++) {
            *pkt = *iter;
            this->get_internal(*pkt);
            pthread_mutex_unlock(&_mutex);
            ret = ERR_OK;
            break;
        }
    }
    pthread_mutex_unlock(&_mutex);
    return ret;
}

int Queue::peek(void **pkt, bool block) {
    int ret = ERR_OK;
    
    pthread_mutex_lock(&_mutex);

    for (;;) {
        if (_abort_request) {
            ret = CBERROR(ERR_EIO);
            break;
        }
        if (_signal) {
            _signal = false;
            ret = ERR_AGAIN;
            break;
        }
        if (!_queue.empty()) {
            *pkt = _queue.front();
            this->get_internal(*pkt);
            ret = ERR_OK;
            break;
        }
        else if (!block) {
            ret = ERR_AGAIN;
            break;
        } else {
            pthread_cond_wait(&_cond, &_mutex);
        }
    }
    pthread_mutex_unlock(&_mutex);
    return ret;
}

int Queue::get_at(void **pkt, int pos) {
    int i = 0;
    pthread_mutex_lock(&_mutex);

    if (_queue.empty()) {
        return CBERROR(ERR_INVAL);
    }
    for (list<void *>::iterator iter = _queue.begin(); iter != _queue.end(); ++iter) {
        if (pos == i++) {
            *pkt = *iter;
            this->get_internal(*pkt);
            _queue.erase(iter);
            pthread_cond_signal&_cond);
            return ERR_OK;
        }
    }

    return CBERROR(ERR_INVAL);
}

int Queue::get(void **pkt, bool block) {
    int ret = ERR_OK;
    
    pthread_mutex_lock(&_mutex);

    for (;;) {
        if (_abort_request) {
            ret = CBERROR(ERR_EIO);
            break;
        }
        if (_signal) {
            _signal = false;
            printf("[%s:%d]===signal\n", __FUNCTION__, __LINE__);
            ret = ERR_AGAIN;
            break;
        }
        if(!_queue.empty()) {
            *pkt = _queue.front();
            _queue.erase(_queue.begin());
            this->get_internal(*pkt);
            pthread_cond_signal&_cond);
            ret = ERR_OK;
            break;
        }
        else if (!block) {
            ret = ERR_AGAIN;
            break;
        } else {
            //av_log(NULL, AV_LOG_DEBUG, "[%s:%d] wait\n", __FUNCTION__, __LINE__);
            pthread_cond_wait(&_cond, &_mutex);
            //av_log(NULL, AV_LOG_DEBUG, "[%s:%d] wait out\n", __FUNCTION__, __LINE__);
        }
    }
    pthread_mutex_unlock(&_mutex);
    return ret;
}

int Queue::size() {
    int size = 0;
    pthread_mutex_lock(&_mutex);
    size = _queue.size();
    pthread_mutex_unlock(&_mutex);
    return size;
}

int Queue::get_internal(void *pkt) {
    return 0;
}

int Queue::put_internal(void *pkt) {
    return 0;
}



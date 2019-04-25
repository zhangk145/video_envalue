
#ifndef UTILS_CBERROR_H
#define UTILS_CBERROR_H

enum ErrRet{
    ERR_OK         = 0,
    ERR_AGAIN,
    ERR_EOF        = 10000,
    ERR_EIO,
    ERR_INVAL,
    ERR_UNKNOWN,
    ERR_MAX
};

#define CBERROR(e) (-(e))

#endif


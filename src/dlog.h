#ifndef DLOG_H
#define DLOG_H
#include "tools.h"
#include "dthread.h"

typedef struct dLogMessage dLogMessage;
struct dLogMessage{
    char msg[128];
    u32 msg_len; 
};

#define MAX_LOGS 8 
typedef struct dLogger dLogger;
struct dLogger{
    dLogMessage logs[MAX_LOGS];
    u32 current_log;

    dMutex log_mutex;
    b32 console_log; //prints logs to console as soon as they come
};

b32 dlog_init(dLogger *logger);
b32 dlog(dLogger *logger, char *format, ...);

#endif
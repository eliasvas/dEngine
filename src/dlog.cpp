#include  "dlog.h"
#include "stdarg.h"


dLogger engine_log;


b32 dlog_init(dLogger *logger)
{
    if (logger == NULL)logger = &engine_log;
    logger->current_log = 0;
    logger->log_mutex = dmutex_create();
    logger->console_log = TRUE;//FALSE;
    return TRUE;
}

b32 dlog(dLogger *logger, char *format, ...)
{
    if (logger == NULL)logger = &engine_log;
    dmutex_lock(&logger->log_mutex);
    dLogMessage *msg = &logger->logs[logger->current_log++ % MAX_LOGS];
    va_list args;
    va_start (args, format);
    //sprintf(msg->msg, format,args); 
    vsnprintf (msg->msg,128,format, args);
    va_end(args);
    msg->msg_len = str_size(msg->msg);

    if (logger->console_log)
        printf(msg->msg);
    dmutex_unlock(&logger->log_mutex);
}
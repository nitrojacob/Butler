#ifndef __TRAP_H__
#define __TRAP_H__

#include <stdarg.h>

/**
 * @brief Initialize trap module and register clearButton_cb with stateProbe
 */
void trap_init(void);

/**
 * @brief Initialize trap module after stateProbe init
 */
void trap_late_init(void);

/*
 * Butlers logging helper
 * Usage: BUTLER_LOG("Hello %s", name);
 * The macro forwards to trap_butler_log which formats the message, prefixes
 * a timestamp in yyyy-mm-dd HH:MM format and writes the entry to the
 * non-volatile ring and to stateProbe_log.
 */
void trap_butler_log(const char* fmt, ...);

#define BUTLER_LOG(fmt, ...) trap_butler_log((fmt), ##__VA_ARGS__)

#endif /* __TRAP_H__ */

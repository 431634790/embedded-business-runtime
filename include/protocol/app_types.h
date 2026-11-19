#ifndef EMBEDDED_BUSINESS_RUNTIME_APP_TYPES_H
#define EMBEDDED_BUSINESS_RUNTIME_APP_TYPES_H

#include <stddef.h>
#include <stdint.h>

/* All translation units must use the same capacity. No heap storage is used. */
#ifndef APP_MSG_PAYLOAD_CAPACITY
#define APP_MSG_PAYLOAD_CAPACITY 128U
#endif

#if APP_MSG_PAYLOAD_CAPACITY < 1
#error "APP_MSG_PAYLOAD_CAPACITY must be at least 1"
#endif

/*
 * Queues own a complete value copy, including the inline payload.
 * msg_id: application-defined message/event identifier.
 * payload: inline bytes; do not embed pointers to temporary/driver-owned data.
 * len: valid byte count, from 0 to APP_MSG_PAYLOAD_CAPACITY inclusive.
 * Producers initialize messages (e.g. app_msg_t msg = {0}). Queue APIs must
 * reject oversized len values before copying or accessing payload bytes.
 * This is an in-process contract, not an on-wire serialization format.
 */
typedef struct app_msg {
    uint32_t msg_id;
    uint8_t payload[APP_MSG_PAYLOAD_CAPACITY];
    size_t len;
} app_msg_t;

/* Lifecycle hooks return 0 on success, nonzero on failure. */
typedef int (*app_module_hook_t)(void);

/*
 * Descriptors and names must remain valid for the entire runtime lifetime;
 * use static storage. Hooks run only on the core event-loop thread.
 * init: initialize module-owned static state and register dependencies.
 * ready: perform cross-module wiring after EVERY module's init succeeds.
 * A NULL hook is a no-op. Any init failure prevents the entire ready phase.
 * Any ready failure prevents entry into the normal event loop.
 * Module registration is frozen before the init phase begins.
 */
typedef struct app_module {
    const char *name;
    app_module_hook_t init;
    app_module_hook_t ready;
} app_module_t;

#endif /* EMBEDDED_BUSINESS_RUNTIME_APP_TYPES_H */

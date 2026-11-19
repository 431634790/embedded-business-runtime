#ifndef EMBEDDED_BUSINESS_RUNTIME_APP_RUNTIME_H
#define EMBEDDED_BUSINESS_RUNTIME_APP_RUNTIME_H

#include "protocol/app_types.h"

/* Override consistently for every translation unit at build time. */
#ifndef APP_RUNTIME_MAX_MODULES
#define APP_RUNTIME_MAX_MODULES 16
#endif

#if APP_RUNTIME_MAX_MODULES < 1
#error "APP_RUNTIME_MAX_MODULES must be at least 1"
#endif

/* Reserved framework errors; module hooks should not return these values. */
enum app_runtime_result {
    APP_RUNTIME_OK = 0,
    APP_RUNTIME_ERR_ARGUMENT = -1001,
    APP_RUNTIME_ERR_CAPACITY = -1002,
    APP_RUNTIME_ERR_STARTED = -1003,
    APP_RUNTIME_ERR_PLATFORM = -1004
};

/*
 * Register a fixed module table and run the core on the CALLING thread:
 *   all init hooks -> all ready hooks -> event_loop_run().
 * The function blocks until a hook fails or the event loop returns.
 * Hooks run in table order; NULL hooks are skipped. The first nonzero hook
 * result is returned unchanged. Otherwise return the event loop's result.
 *
 * count must be in [0, APP_RUNTIME_MAX_MODULES]. NULL modules is allowed only
 * when count is zero. Names must be nonempty, unique, and valid for the entire
 * runtime lifetime (normally string literals). The table must not be modified
 * concurrently with this call. Descriptors are copied into static storage
 * before callbacks run; callback edits to the original table have no effect.
 *
 * Only one valid startup attempt is allowed per process, including concurrent
 * or recursive calls. This remains true after a hook fails or the loop exits:
 * no cleanup/rollback hooks exist yet, so retrying could duplicate resources.
 * Invalid arguments/capacity errors do not consume the startup attempt.
 * No worker thread or dynamic storage is created by the module manager.
 */
int runtime_start(const app_module_t *modules, int count);

#endif /* EMBEDDED_BUSINESS_RUNTIME_APP_RUNTIME_H */

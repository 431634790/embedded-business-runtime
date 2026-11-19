#include "framework/app_runtime.h"
#include "event_loop.h"

#include <pthread.h>
#include <string.h>

static app_module_t module_table[APP_RUNTIME_MAX_MODULES];
static pthread_mutex_t startup_mutex = PTHREAD_MUTEX_INITIALIZER;
static int startup_claimed;

/* Called with startup_mutex held, before any hook or state change. */
static int validate_modules(const app_module_t *modules, int count)
{
    int i;
    int j;

    if (count < 0 || (count > 0 && modules == NULL)) {
        return APP_RUNTIME_ERR_ARGUMENT;
    }
    if (count > APP_RUNTIME_MAX_MODULES) {
        return APP_RUNTIME_ERR_CAPACITY;
    }

    for (i = 0; i < count; ++i) {
        if (modules[i].name == NULL || modules[i].name[0] == '\0') {
            return APP_RUNTIME_ERR_ARGUMENT;
        }
        for (j = 0; j < i; ++j) {
            if (strcmp(modules[i].name, modules[j].name) == 0) {
                return APP_RUNTIME_ERR_ARGUMENT;
            }
        }
    }
    return APP_RUNTIME_OK;
}

int runtime_start(const app_module_t *modules, int count)
{
    int i;
    int result;

    if (pthread_mutex_lock(&startup_mutex) != 0) {
        return APP_RUNTIME_ERR_PLATFORM;
    }

    if (startup_claimed) {
        result = APP_RUNTIME_ERR_STARTED;
    } else {
        result = validate_modules(modules, count);
        if (result == APP_RUNTIME_OK) {
            for (i = 0; i < count; ++i) {
                module_table[i] = modules[i];
            }
            startup_claimed = 1;
        }
    }

    if (pthread_mutex_unlock(&startup_mutex) != 0) {
        return APP_RUNTIME_ERR_PLATFORM;
    }
    if (result != APP_RUNTIME_OK) {
        return result;
    }

    /* Never hold the startup lock across user code or the blocking loop. */
    for (i = 0; i < count; ++i) {
        if (module_table[i].init != NULL) {
            result = module_table[i].init();
            if (result != APP_RUNTIME_OK) {
                return result;
            }
        }
    }

    /* A hard phase boundary: no ready hook runs until ALL init hooks pass. */
    for (i = 0; i < count; ++i) {
        if (module_table[i].ready != NULL) {
            result = module_table[i].ready();
            if (result != APP_RUNTIME_OK) {
                return result;
            }
        }
    }

    return event_loop_run();
}

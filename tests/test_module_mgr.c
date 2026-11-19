#include "framework/app_runtime.h"
#include "event_loop.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        exit(EXIT_FAILURE); \
    } \
} while (0)

static char calls[32];
static size_t call_count;
static pthread_t core_thread;
static int init_b_result;
static int ready_a_result;
static const int loop_result = 23;

static void record(char call)
{
    CHECK(pthread_equal(pthread_self(), core_thread));
    CHECK(call_count + 1 < sizeof(calls));
    calls[call_count++] = call;
    calls[call_count] = '\0';
}

static int init_a(void) { record('A'); return 0; }
static int init_b(void) { record('B'); return init_b_result; }
static int ready_a(void) { record('a'); return ready_a_result; }
static int ready_b(void) { record('b'); return 0; }

int event_loop_run(void)
{
    record('E');
    return loop_result;
}

static app_module_t modules[] = {
    { "thermal", init_a, ready_a },
    { "no-hooks", NULL, NULL },
    { "ui", init_b, ready_b }
};

static int recursive_init(void)
{
    CHECK(runtime_start(NULL, 0) == APP_RUNTIME_ERR_STARTED);
    return init_a();
}

static int unexpected_hook(void)
{
    CHECK(0);
    return 1;
}

static int mutate_source_init(void)
{
    modules[2].init = unexpected_hook;
    modules[2].ready = unexpected_hook;
    return init_a();
}

static pthread_mutex_t gate_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t gate_condition = PTHREAD_COND_INITIALIZER;
static int hook_entered;
static int hook_released;
static int worker_result;

static int paused_init(void)
{
    record('A');
    CHECK(pthread_mutex_lock(&gate_mutex) == 0);
    hook_entered = 1;
    CHECK(pthread_cond_signal(&gate_condition) == 0);
    while (!hook_released) {
        CHECK(pthread_cond_wait(&gate_condition, &gate_mutex) == 0);
    }
    CHECK(pthread_mutex_unlock(&gate_mutex) == 0);
    return 0;
}

static void *start_worker(void *unused)
{
    (void)unused;
    core_thread = pthread_self();
    worker_result = runtime_start(modules, 3);
    return NULL;
}

static void test_concurrent(void)
{
    pthread_t worker;

    modules[0].init = paused_init;
    CHECK(pthread_create(&worker, NULL, start_worker, NULL) == 0);
    CHECK(pthread_mutex_lock(&gate_mutex) == 0);
    while (!hook_entered) {
        CHECK(pthread_cond_wait(&gate_condition, &gate_mutex) == 0);
    }
    CHECK(pthread_mutex_unlock(&gate_mutex) == 0);

    /* Another caller cannot execute hooks or start a second event loop. */
    CHECK(runtime_start(NULL, 0) == APP_RUNTIME_ERR_STARTED);

    CHECK(pthread_mutex_lock(&gate_mutex) == 0);
    hook_released = 1;
    CHECK(pthread_cond_signal(&gate_condition) == 0);
    CHECK(pthread_mutex_unlock(&gate_mutex) == 0);
    CHECK(pthread_join(worker, NULL) == 0);
    CHECK(worker_result == loop_result);
    CHECK(strcmp(calls, "ABabE") == 0);
}

static void test_invalid(void)
{
    app_module_t unnamed = { NULL, init_a, ready_a };
    app_module_t empty_name = { "", init_a, ready_a };
    app_module_t duplicates[] = {
        { "duplicate", init_a, ready_a },
        { "duplicate", init_b, ready_b }
    };

    CHECK(runtime_start(NULL, -1) == APP_RUNTIME_ERR_ARGUMENT);
    CHECK(runtime_start(NULL, 1) == APP_RUNTIME_ERR_ARGUMENT);
    CHECK(runtime_start(&unnamed, 1) == APP_RUNTIME_ERR_ARGUMENT);
    CHECK(runtime_start(&empty_name, 1) == APP_RUNTIME_ERR_ARGUMENT);
    CHECK(runtime_start(duplicates, 2) == APP_RUNTIME_ERR_ARGUMENT);
    CHECK(call_count == 0);
    CHECK(runtime_start(NULL, 0) == loop_result);
    CHECK(strcmp(calls, "E") == 0);
}

static void test_capacity(void)
{
    static app_module_t full_table[APP_RUNTIME_MAX_MODULES];
    static char names[APP_RUNTIME_MAX_MODULES][32];
    int i;

    CHECK(runtime_start(modules, APP_RUNTIME_MAX_MODULES + 1)
          == APP_RUNTIME_ERR_CAPACITY);
    CHECK(call_count == 0);
    for (i = 0; i < APP_RUNTIME_MAX_MODULES; ++i) {
        CHECK(snprintf(names[i], sizeof(names[i]), "module-%d", i) > 0);
        full_table[i].name = names[i];
    }
    CHECK(runtime_start(full_table, APP_RUNTIME_MAX_MODULES) == loop_result);
    CHECK(strcmp(calls, "E") == 0);
}

int main(int argc, char **argv)
{
    CHECK(argc == 2);
    core_thread = pthread_self();

    if (strcmp(argv[1], "order") == 0) {
        CHECK(runtime_start(modules, 3) == loop_result);
        CHECK(strcmp(calls, "ABabE") == 0);
    } else if (strcmp(argv[1], "init_failure") == 0) {
        app_module_t failure_modules[] = {
            { "thermal", init_a, ready_a },
            { "ui", init_b, ready_b },
            { "must-not-run", unexpected_hook, unexpected_hook }
        };
        init_b_result = 7;
        CHECK(runtime_start(failure_modules, 3) == 7);
        CHECK(strcmp(calls, "AB") == 0);
    } else if (strcmp(argv[1], "ready_failure") == 0) {
        ready_a_result = -17;
        CHECK(runtime_start(modules, 3) == -17);
        CHECK(strcmp(calls, "ABa") == 0);
    } else if (strcmp(argv[1], "empty") == 0) {
        CHECK(runtime_start(NULL, 0) == loop_result);
        CHECK(strcmp(calls, "E") == 0);
    } else if (strcmp(argv[1], "invalid") == 0) {
        test_invalid();
    } else if (strcmp(argv[1], "capacity") == 0) {
        test_capacity();
    } else if (strcmp(argv[1], "reentrant") == 0) {
        modules[0].init = recursive_init;
        CHECK(runtime_start(modules, 3) == loop_result);
        CHECK(strcmp(calls, "ABabE") == 0);
    } else if (strcmp(argv[1], "concurrent") == 0) {
        test_concurrent();
    } else if (strcmp(argv[1], "snapshot") == 0) {
        modules[0].init = mutate_source_init;
        CHECK(runtime_start(modules, 3) == loop_result);
        CHECK(strcmp(calls, "ABabE") == 0);
    } else {
        CHECK(0);
    }

    /* Neither successful exit nor partial initialization permits a retry. */
    CHECK(runtime_start(NULL, 0) == APP_RUNTIME_ERR_STARTED);
    printf("PASS %s\n", argv[1]);
    return EXIT_SUCCESS;
}

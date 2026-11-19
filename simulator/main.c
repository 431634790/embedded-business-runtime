#include "framework/app_runtime.h"
#include "event_loop.h"

#include <stdio.h>

/* Temporary link provider, not the final blocking message-queue loop. */
int event_loop_run(void)
{
    puts("Lifecycle startup complete; the event loop is not implemented yet.");
    return 0;
}

/* Business modules and simulator driver/UI threads are added in later stages. */
int main(void)
{
    return runtime_start(NULL, 0) == APP_RUNTIME_OK ? 0 : 1;
}

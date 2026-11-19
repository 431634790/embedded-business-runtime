#ifndef EMBEDDED_BUSINESS_RUNTIME_EVENT_LOOP_H
#define EMBEDDED_BUSINESS_RUNTIME_EVENT_LOOP_H

/*
 * Private core integration point. Runs on runtime_start's calling thread,
 * after all init and ready hooks succeed. Return 0 for a normal exit or a
 * nonzero failure. The simulator currently supplies an explicit placeholder;
 * replace it with the real message-queue event loop in a later stage.
 */
int event_loop_run(void);

#endif /* EMBEDDED_BUSINESS_RUNTIME_EVENT_LOOP_H */

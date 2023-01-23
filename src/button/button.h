#ifndef _BUTTON_H_
#define _BUTTON_H_

#define DEBOUNCE_FILTER_MS              100
#define MULTI_CLICK_MS                  600
#define LONG_PRESSED_MS                 2000
#define MAX_KEYS                        10

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum key_event_e {
    KEY_EVENT_CLICK = 1,
    KEY_EVENT_MULT_CLICK = 2,
    KEY_EVENT_PRESSED = 4,
    KEY_EVENT_LONG_PRESSED = 8,
}pin_event_t;

typedef void (*pinEventHandler_t)(int id, pin_event_t eventType);
typedef int (*pinReadFunction_t)(int id);

typedef struct pin_s {
    unsigned id; //1-255
    bool idleState;
    pinEventHandler_t handler;
    pinReadFunction_t read_func;
    bool delay;
}pin_t;

bool addButton(pin_t *btn);
void buttonEventLoop(void);


#ifdef __cplusplus
}
#endif

#endif
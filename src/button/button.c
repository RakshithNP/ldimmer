#include "button.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum key_state_e
{
    KEY_STATE_IDLE = 0,
    KEY_STATE_DEBOUNCE,
    KEY_STATE_PRESSED,
    KEY_STATE_RELEASED,
    KEY_STATE_SENTIMEL
} pin_state_t;

struct pin_meta_s
{
    pin_t *key;
    pin_state_t state;
    unsigned eventsDispatched;

    unsigned long lastTick;
    unsigned long lastReleasedTick;
    unsigned totalClicks;
    unsigned multiClicks;
    bool lastGPIOState;
    bool delayClicked;
};

static struct pin_meta_s key_meta[MAX_KEYS];
static unsigned key_meta_len = 0;

unsigned long millis(void)__attribute__((weak));

bool addButton(pin_t *key)
{
    if (!key)
        return false;
    if (key_meta_len >= MAX_KEYS)
        return false;

    if(key->id==0)
    {
        return false;
    }

    //Checking for EXisting ID
    for (int i = 0; i < key_meta_len; i++)
    {
        if(key_meta[i].key!=NULL)
        {
            if(key_meta[i].key->id==key->id)
            {
                return false;
            }
        }
    }

    //Pins are considered to be configured as input
    //gpio_mode(key->port, key->pin, GPIO_INPUT_MODE);
    //    enablePullUp(key->port, key->pin);

    key_meta[key_meta_len].key = key;
    key_meta[key_meta_len].state = KEY_STATE_IDLE;
    key_meta[key_meta_len].eventsDispatched = 0;

    key_meta[key_meta_len].lastTick = millis();
    key_meta[key_meta_len].lastReleasedTick = millis();
    key_meta[key_meta_len].totalClicks = 0;
    key_meta[key_meta_len].multiClicks = 0;
    key_meta[key_meta_len].delayClicked = false;
    if (key->read_func != NULL)
    {
        key_meta[key_meta_len].lastGPIOState = key->read_func(key->id);
    }
    key_meta_len++;
    return true;
}

void dispatchEvent(struct pin_meta_s *pMeta, pin_event_t eventType)
{
    const pin_t *key = pMeta->key;
    if (pMeta->eventsDispatched & eventType)
        return;
    if ((pMeta->eventsDispatched & KEY_EVENT_LONG_PRESSED) && (eventType == KEY_EVENT_CLICK))
        return;
    if ((pMeta->eventsDispatched & KEY_EVENT_LONG_PRESSED) && (eventType == KEY_EVENT_MULT_CLICK))
        return;
    if (key->handler)
        key->handler(key->id,eventType);

    pMeta->eventsDispatched |= eventType;
}

void buttonEventLoop(void)
{
    unsigned i;
    for (i = 0; i < key_meta_len; i++)
    {
        struct pin_meta_s *pMeta = key_meta + i;
        const pin_t *key = pMeta->key;

        bool newState = key->read_func(key->id);
        bool lastState = pMeta->lastGPIOState;

        //        if (pMeta->state != KEY_STATE_IDLE)
        //            LOGLINE_DEBUG("State: %u pin %u", pMeta->state, key.pin);

        if (pMeta->delayClicked && (millis() - pMeta->lastReleasedTick) > MULTI_CLICK_MS)
        {
            if (key->handler)
                key->handler(key->id,KEY_EVENT_CLICK);
            pMeta->delayClicked = false;
            pMeta->lastReleasedTick = millis();
        }

        switch (pMeta->state)
        {
        case KEY_STATE_IDLE:
        {
            if ((lastState == newState) && (newState == key->idleState))
                break;

            pMeta->eventsDispatched = 0;
            pMeta->state = KEY_STATE_DEBOUNCE;
            // Fall Through
        }
        case KEY_STATE_DEBOUNCE:
        {
            if (lastState != newState)
                pMeta->lastTick = millis();
            if ((millis() - pMeta->lastTick) < DEBOUNCE_FILTER_MS)
                break;
            if (newState == key->idleState)
            {
                pMeta->state = KEY_STATE_RELEASED;
                break;
            }

            pMeta->state = KEY_STATE_PRESSED;
            // Fall Though
        }
        case KEY_STATE_PRESSED:
        {
            if (newState == key->idleState)
            {
                pMeta->state = KEY_STATE_DEBOUNCE;
                if (key->delay && !(pMeta->eventsDispatched & KEY_EVENT_LONG_PRESSED))
                    dispatchEvent(pMeta, KEY_EVENT_PRESSED);
                break;
            }

            if (!key->delay)
                dispatchEvent(pMeta, KEY_EVENT_PRESSED);
            // Checking if Long Pressed!
            if (millis() - pMeta->lastTick >= (LONG_PRESSED_MS + DEBOUNCE_FILTER_MS))
            {
                dispatchEvent(pMeta, KEY_EVENT_LONG_PRESSED);
                pMeta->lastTick = millis();
            }
            break;
        }
        case KEY_STATE_RELEASED:
        {
            // Checking if Multi-Clicked!
            if (millis() - pMeta->lastReleasedTick <= MULTI_CLICK_MS)
            {
                dispatchEvent(pMeta, KEY_EVENT_MULT_CLICK);
                pMeta->multiClicks++;
                pMeta->delayClicked = false;
            }
            else
            {
                if (key->delay && !(pMeta->eventsDispatched & KEY_EVENT_LONG_PRESSED))
                    pMeta->delayClicked = true;
                else
                    dispatchEvent(pMeta, KEY_EVENT_CLICK);
                pMeta->multiClicks = 0;
            }

            pMeta->totalClicks++;
            pMeta->lastReleasedTick = millis();
            pMeta->state = KEY_STATE_IDLE;
            break;
        }
        default:
        {
            break;
        }
        }
        pMeta->lastGPIOState = newState;
    }
}
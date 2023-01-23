// This program is for single module dimmer which has a dual module top pcb hardware to control the brightness 
// long press the bottom button to turn off the dimmer immediately 
#include "Modlib_EEPROM_Config.h"
#include "src/button/button.h"
#include "src/modlib/I2C.h"
#include "src/triac_controller/triac_controller.h"
#include <EEPROM.h>
#include <Wire.h>
/** firmware version */
#define FIRMWARE_V_C 1.0

#define TRIAC_PIN 2      // triac trigger pin
#define ZCD_DETECT_PIN 3 // zcd input as interrupt

/** TOP and Bottom Buttons*/
#define TOP_BUTTON_PIN 8    // top push button pin
#define BOTTOM_BUTTON_PIN 7 // bottom push botton pin
/** Button LEDs */
#define TOP_LED_PIN_1 10   // top led 1 pin white
#define BOTTOM_LED_PIN_1 5 // bottom led 1 pin white
#define TOP_LED_PIN_2 6    // top led 2 pin blue
#define BOTTOM_LED_PIN_2 9 // bottom led 2 pin blue
/** EEPROM address */
#define DEVICE_ADDRESS 10     // eeprom address of device control
#define DIM_ADDRESS 11        // eeprom address of dim level
#define DIM_MAX_ADDRESS 12    // eeprom address of dim max
#define DIM_MIN_ADDRESS 13    // eeprom address of dim min
#define DIM_ON_OFF_ADDRESS 14 // eeprom address for dimmer on off
/** Dimming Default Values*/
#define MAX_DIMVALUE 100 // absolute max limit of dim level
#define MIN_DIMVALUE 10  // absolute min limit of dim level
/**Vsense pin */
#define VSENS_PIN A1 // pairing pin
/** Leds Brightness */
#define LEAST_LED_BRIGHTNESS 1    // least led brightness of button led
#define DEFAULT_LED_BRIGHTNESS 25 // Default led brightness of button led

/** Application Global Variable */
volatile int dimTime; // dim time calculated by (75 * (dim_level))
volatile byte dimValue, dimValue_temp;           // dim level var
volatile byte dimMax, dimMax_temp;               // max dim level var
volatile byte dimMin, dimMin_temp;               // min dim level var
volatile byte deviceControl, deviceControl_temp; // device control state var
volatile byte change;                            // change in any of above vars
volatile uint8_t dim_on_off, dim_on_off_temp;
uint8_t button_led_brightness = DEFAULT_LED_BRIGHTNESS,
        button_led_brightness_temp =
            DEFAULT_LED_BRIGHTNESS; // button led brightness level

// triac config
struct triac_ctrl_config t1_conf;
// triac param
struct triac_ctrl_param t1_params;
// Triac instances
struct triac_unit t1 = {.config = &t1_conf, .params = &t1_params};

/** Log variable */
uint8_t logs[70] = {0};

/* Pin related callbacks function prototype*/
void top_button_event_handler(int id, pin_event_t eventType);
int top_pin_read_fucn(int id);
void bottom_button_event_handler(int id, pin_event_t eventType);
int bottom_pin_read_fucn(int id);

/*some function prototype*/
static void start_period_timer(void);

/** Declairation */
pin_t top_button = {.id = 1,
                    .idleState = 0,
                    .handler = top_button_event_handler,
                    .read_func = top_pin_read_fucn};
pin_t bottom_button = {.id = 2,
                       .idleState = 0,
                       .handler = bottom_button_event_handler,
                       .read_func = bottom_pin_read_fucn};
//  ----- zero cross interrupt ISR  ----- //
void zero_crosss_INT() {
  /*check if dim is on */
  if (dim_on_off != 1) {
    /* else return */
    return;
  }
  /* Proceed for triac controll */
  delayMicroseconds(dimTime); // waite for time period of dimTime in microsends
  digitalWrite(TRIAC_PIN, HIGH); // trigger triac high
  delayMicroseconds(10); // minimum time to trigger triac high (rise time)
  digitalWrite(TRIAC_PIN, LOW); // trigger triac low
  /* Turn on Period timer*/
  // start_period_timer();
}
/**
 * @brief
 *
 * @param id
 * @param eventType
 */
void top_button_event_handler(int id, pin_event_t eventType) {
  if (eventType == KEY_EVENT_CLICK) {
    /* increase dim value*/
    Serial.println("top pin clicked");
    if (dim_on_off_temp == 0) {
      Serial.println("Turning on Dimming ");
      dim_on_off_temp = 1;
    } else {
      Serial.print("last dimm value was :");
      Serial.println(dimValue_temp);
      dimValue_temp += 10;                                                        // increase dimm value by 10
      if (dimValue_temp > 100) {
        dimValue_temp = 100;
      }
    }
  } else if (eventType == KEY_EVENT_LONG_PRESSED) {
    /* Turn off dimming */
    Serial.println("top pin long pressed");
  }
}
/**
 * @brief
 *
 * @param id
 * @return int
 */
int top_pin_read_fucn(int id) { return digitalRead(TOP_BUTTON_PIN); }
/**
 * @brief
 *
 * @param id
 * @param eventType
 */
void bottom_button_event_handler(int id, pin_event_t eventType) {
  if (eventType == KEY_EVENT_CLICK) {
    /* decrease dim value*/
    Serial.println("Bottom pin clicked");
    Serial.print("last dimm value was :");
    Serial.println(dimValue_temp);
    int a = dimValue_temp - 10;
    if (a < 0) {
      return;
    }
    dimValue_temp -= 10;                                                      // decrease dim value by 10 
  } else if (eventType == KEY_EVENT_LONG_PRESSED) {
    /* Turn off dimming */
    Serial.println("Bottom pin long pressed");
    if (dim_on_off_temp == 1) {
      Serial.println("Turning off Dimming ");
      dim_on_off_temp = 0;
    }
  }
}
/**
 * @brief
 *
 * @param id
 * @return int
 */
int bottom_pin_read_fucn(int id) { return digitalRead(BOTTOM_BUTTON_PIN); }
/**
 * @brief Set the up timer object
 *
 * initialize timer in CTC Mode with 55us
 */
static void setup_timer(void) {
  // Setting Timer 1 in normal mode
  TCCR1A = 0;
  TCNT1 = 0;
  // Every Perioad will be of 55us
  OCR1A = 15;
  // Cloack at 1 MHZ
  TCCR1B |= (1 << 0); //|(1<<1);
  // Enable Interrupt A Compare
  TIMSK1 |= (1 << OCIE1A);
}
// Zero crossing callback function
static bool get_t1_zc(void) { return (digitalRead(ZCD_DETECT_PIN) == 1); }
//
static void set_t1_trigger(bool value) {
  digitalWrite(TRIAC_PIN, value ? HIGH : LOW);
}
/**
 * @brief
 *
 * @param dimvalue
 * @return float
 */
static float dimvalue_to_angle(int dimvalue) {
  /* calculating phase angle*/
  float temp_angle =
      MIN_ALLOWE_TRIGGER_ANGLE +
      (MAX_ALLOWED_TRIGGER_ANGLE - MIN_ALLOWE_TRIGGER_ANGLE) * dimValue / 100;
  /* inverting */
  temp_angle = 180.0f - temp_angle;
  /* Limiting Upper value*/
  if (temp_angle > MAX_ALLOWED_TRIGGER_ANGLE) {
    temp_angle = MAX_ALLOWED_TRIGGER_ANGLE;
  }
  /* Limiting Lower Value*/
  if (temp_angle < MIN_ALLOWE_TRIGGER_ANGLE) {
    temp_angle = MIN_ALLOWE_TRIGGER_ANGLE;
  }
  return temp_angle;
}
/**
 * @brief start notification
 *
 */
static void startNotification() {
  if (eeprom_read_byte(MEM_SLAVE_FLAG) == SLAVE_ADDRESS_FLAG) {
    for (uint8_t i = 0; i < 2; i++) {
      for (uint8_t k = 1; k < 25; k++) {
        analogWrite(TOP_LED_PIN_1, k);
        analogWrite(TOP_LED_PIN_2, k);
        analogWrite(BOTTOM_LED_PIN_1, k);
        analogWrite(BOTTOM_LED_PIN_2, k);
        delay(50);
      }
      delay(50);
      for (uint8_t k = 25; k > 0; k--) {
        analogWrite(TOP_LED_PIN_1, k);
        analogWrite(TOP_LED_PIN_2, k);
        analogWrite(BOTTOM_LED_PIN_1, k);
        analogWrite(BOTTOM_LED_PIN_2, k);
        delay(50);
      }
      delay(50);
    }
  } else {
    for (uint8_t i = 0; i < 4; i++) {
      analogWrite(TOP_LED_PIN_1, 255);
      analogWrite(BOTTOM_LED_PIN_1, 255);
      analogWrite(TOP_LED_PIN_2, 255);
      analogWrite(BOTTOM_LED_PIN_2, 255);
      delay(250);
      analogWrite(TOP_LED_PIN_1, 0);
      analogWrite(BOTTOM_LED_PIN_1, 0);
      analogWrite(TOP_LED_PIN_2, 0);
      analogWrite(BOTTOM_LED_PIN_2, 0);
      delay(250);
    }
  }
}
/**
 * @brief I2c pairing methode
 *
 */
static void isI2CPair() {
  unsigned long time_now = millis();
  unsigned long pressed = 0;
  bool state = digitalRead(VSENS_PIN);
  uint8_t count = 0;
  while ((millis() - time_now) < 500) // read pulse train on VENS_PIN
  {
    bool change = digitalRead(VSENS_PIN);
    if (change != state &&
        ((millis() - pressed) >= 20 && (millis() - pressed) <= 100)) {
      pressed = millis();
      state = change;
      count++;
      if (count >= 5) {
        break;
      }
    }
  }
  if (count >= 5) // if VSENS_PIN pulse count is more than 5
  {
    eeprom_write_byte(MEM_SLAVE_FLAG, 0xFF);
    I2C.pairingWait(); // intialize i2c pairing port at address 10
    unsigned long time_now = millis();
    while ((millis() - time_now) < 1000) {
      if (eeprom_read_byte(MEM_SLAVE_FLAG) ==
          SLAVE_ADDRESS_FLAG) // if eeprom slave flag is similar to slave
                              // address flag means its paired and exit while
                              // loop
      {
        break;
      }
      delay(100);
    }
    I2C.stop(); // disable i2c
  }
}
//  ----- setup ----- //

void setup() {
  /* Configure Pins */
  pinMode(TRIAC_PIN, OUTPUT);        // triac trigger pin as outout
  pinMode(ZCD_DETECT_PIN, INPUT);    // zcd pin as input
  pinMode(TOP_BUTTON_PIN, INPUT);    // Top Button as input
  pinMode(BOTTOM_BUTTON_PIN, INPUT); // Bottom Button as input
  pinMode(TOP_LED_PIN_1, OUTPUT);    // Top button led 1
  pinMode(BOTTOM_LED_PIN_1, OUTPUT); // Bottom Button led 1
  pinMode(TOP_LED_PIN_2, OUTPUT);    // Top Button led 2
  pinMode(BOTTOM_LED_PIN_2, OUTPUT); // Bottom Button led 2

  //Serial.begin(9600);

  /* Write Default Values to output pins*/
  digitalWrite(TRIAC_PIN, LOW);      // turn off triac on startup
  digitalWrite(TOP_LED_PIN_1, 0);    // turn off top button led 1 on startup
  digitalWrite(BOTTOM_LED_PIN_1, 0); // turn off bottom button led 1 on startup
  digitalWrite(TOP_LED_PIN_2, 0);    // turn off top button led 2 on startup
  digitalWrite(BOTTOM_LED_PIN_2, 0); // turn off bottom button led 2 on startup
  /* reset /stop i2c bus */
  I2C.stop();
  /* Check for i2c pairing */
  isI2CPair(); // check for i2c pair

  /* Get EEPROM saved values*/
  dimValue = dimValue_temp =
      EEPROM.read(DIM_ADDRESS); // previous stored value of dim level
  dimMax = dimMax_temp =
      EEPROM.read(DIM_MAX_ADDRESS); // previous stored value of  dim max
  dimMin = dimMin_temp =
      EEPROM.read(DIM_MIN_ADDRESS); // previous stored value of dim min
  // dim_on_off = dim_on_off_temp =
  //     EEPROM.read(DIM_ON_OFF_ADDRESS); // Previous saved on off value

  /* Setting dim Minimum value in range*/
  if (dimMin < MIN_DIMVALUE ||
      dimMin > MAX_DIMVALUE) // check if dim min under absolute min and max
  {
    dimMin = dimMin_temp = MIN_DIMVALUE;
    EEPROM.write(DIM_MIN_ADDRESS, MIN_DIMVALUE);
  }
  /* Setting Dim Maximum value in range*/
  if (dimMax > MAX_DIMVALUE ||
      dimMax < MIN_DIMVALUE) // check if dim max under absolute min and max
  {
    dimMax = dimMax_temp = MAX_DIMVALUE;
    EEPROM.write(DIM_MAX_ADDRESS, MAX_DIMVALUE);
  }

  if ((dimValue < dimMin) ||
      (dimValue > dimMax)) // check if dim value is under dim min and dim max
  {
    dimValue = dimValue_temp = MIN_DIMVALUE;
    EEPROM.write(DIM_ADDRESS, dimValue);
  }
  /* triac config*/
  t1_conf.angle_increment_value = 3.5; // 2.5+execution compensation
  t1_conf.start_trigger_angle = dimvalue_to_angle(dimValue);
  t1_conf.trigger_pulse_lenght = 5;
  t1_conf.get_zc = get_t1_zc;
  t1_conf.set_trigger = set_t1_trigger;
  // Init triac unit
  int ret = triac_ctrl_init(&t1);

  Serial.print("dim min :");
  Serial.println(dimMin);
  Serial.print("dim max :");
  Serial.println(dimMax);
  Serial.print("dim value :");
  Serial.println(dimValue);

  /* ---- NOTE: first assign all i2c events than initialize i2c ----*/
  I2C.firmwareVC = FIRMWARE_V_C; // assign firmeare version, if not assign it
                                 // will be 0 default
  I2C.attachEvent(32, 64, FEATURE_BOOL, &dim_on_off_temp, 0);
  I2C.attachEvent(33, 65, FEATURE_BYTE, &dimValue_temp, MIN_DIMVALUE);
  I2C.attachEvent(34, 66, FEATURE_BYTE, &dimMin_temp, MIN_DIMVALUE);
  I2C.attachEvent(35, 67, FEATURE_BYTE, &dimMax_temp, MAX_DIMVALUE);
  // I2C.attachLog((uint8_t)sizeof(logs), logs);
  I2C.attachLedBrightnessData(&button_led_brightness_temp);
  /* Initialize I2c */
  I2C.init(); // initialize i2c
  /* Start up animation*/
  startNotification(); // power on led sequence
  startNotification(); // power on led sequence
  /* add buttons*/
  addButton(&top_button);
  addButton(&bottom_button);
  /* Start delay */
  delay(1000); // power on delay
  /* Setup timer */
  setup_timer();
  /* Write all led low */
  digitalWrite(TOP_LED_PIN_1, 0);    // turn off top button led 1 on startup
  digitalWrite(BOTTOM_LED_PIN_1, 0); // turn off bottom button led 1 on startup
  digitalWrite(TOP_LED_PIN_2, 0);    // turn off top button led 2 on startup
  digitalWrite(BOTTOM_LED_PIN_2, 0); // turn off bottom button led 2 on startup
}

//  ----- loop  ----- //

void loop() {
  buttonEventLoop();
  /* Check for temp Variable changes */
  /* On off changed */
  if (dim_on_off != dim_on_off_temp) {
    /* Disble Interrupt */
    noInterrupts();
    /* check for turn on */
    if (dim_on_off_temp == 1) {
      /*set dim_on_off*/
      dim_on_off = dim_on_off_temp;
      /* Attach external Interupt*/
      traic_ctrl_turn_on(&t1);
      /*Set dim value*/
      float temp_angle = dimvalue_to_angle(dimValue);
      // dimTime = (75 * dimValue);
      triac_ctrl_set_trigger_angle(&t1, temp_angle);
      /* Write all leds high*/
      analogWrite(TOP_LED_PIN_1, button_led_brightness);
      analogWrite(TOP_LED_PIN_2, button_led_brightness);
      analogWrite(BOTTOM_LED_PIN_1, button_led_brightness);
      analogWrite(BOTTOM_LED_PIN_2, button_led_brightness);
    } else {
      /*set dim_on_off*/
      dim_on_off = dim_on_off_temp;
      /* Turn triac pin Low */
      digitalWrite(TRIAC_PIN, LOW); // trigger triac low
      /* triac control off*/
      triac_ctrl_turn_off(&t1);
      /* Write all the leds low */
      analogWrite(TOP_LED_PIN_1, 0);
      analogWrite(TOP_LED_PIN_2, 0);
      analogWrite(BOTTOM_LED_PIN_1, 0);
      analogWrite(BOTTOM_LED_PIN_2, 0);
    }
    /* Enable interrpt*/
    interrupts();
  }

  /* Dim value changed */
  if (dimValue_temp !=
      dimValue) // check any change in dim level and update memory
  {
    /* Disble Interrupt */
    noInterrupts();
    if(dimValue_temp>=dimMin&&dimValue_temp<=dimMax)
    {
    dimValue = dimValue_temp;
    EEPROM.write(DIM_ADDRESS, dimValue);
    float temp_angle = dimvalue_to_angle(dimValue);
    // dimTime = (75 * dimValue);
    triac_ctrl_set_trigger_angle(&t1, temp_angle);
    Serial.print("Angle: ");
    Serial.println(temp_angle);
    }
    else
    {
      dimValue_temp=dimValue;
    }
    /* Enable interrpt*/
    interrupts();
  }
  /* Dim Max level changed*/
  if (dimMax_temp !=
      dimMax) // check any change in dim max value and update memory
  {
    /* Disble Interrupt */
    noInterrupts();

    if ((dimMax_temp >= MIN_DIMVALUE) && (dimMax_temp <= MAX_DIMVALUE)) {
      dimMax = dimMax_temp;
      EEPROM.write(DIM_MAX_ADDRESS, dimMax);
    } else {
      dimMax_temp = dimMax;
    }
    /* Enable interrpt*/
    interrupts();
  }
  /* Dim Min Level Changed*/
  if (dimMin_temp !=
      dimMin) // check any change in dim min value and update memory
  {
    /* Disble Interrupt */
    noInterrupts();
    if ((dimMin_temp >= MIN_DIMVALUE) && (dimMin_temp <= MAX_DIMVALUE)) {
      dimMin = dimMin_temp;
      EEPROM.write(DIM_MIN_ADDRESS, dimMin);
    } else {
      dimMin_temp = dimMin;
    }
    /* Enable interrpt*/
    interrupts();
  }
}

/* timer interrupt*/
volatile int state = 0;
ISR(TIMER1_COMPA_vect) {
  // Reset flag
  TIFR1 &= ~(1 << 1);
  // Reset Counter to Zero
  TCNT1 = 0;
  //  digitalWrite(TRIAC_PIN,state?LOW:HIGH);
  //  state=state?LOW:HIGH;
  // Run High Frequency task
  triac_ctrl_hf_task_function();
  // digitalWrite(TRIAC_PIN,LOW);
}

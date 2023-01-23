#ifndef I2C_H
#define I2C_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Wire.h>
#include "../../Modlib_EEPROM_Config.h"
#include "modlib_version.h"

#define I2C_REGISTER_SIGN_0 0x4E
#define I2C_REGISTER_SIGN_1 0x45
#define I2C_REGISTER_SIGN_2 0x58
#define I2C_REGISTER_SIGN_3 0x54

#define I2C_READ_SCHEMA_ADDR 0x70
#define I2C_READ_SCHEMA_LEN_ADDR 0x71
#define I2C_READ_DEVICE_ID_ADDR 0x72
#define I2C_READ_HARDWARE_VC_ADDR 0x73
#define I2C_READ_FIRMWARE_VC_ADDR 0x74
#define I2C_READ_LOG_LEN_ADDR 0x75
#define I2C_READ_LOG_ADDR 0x76
#define I2C_READ_BOARD_CONTEXT 0x77
#define I2C_READ_MODLIB_VERSION 0x78

#define I2C_WRITE_FUSE_ADDR 0x80
#define I2C_WRITE_SCHEMA_ADDR 0x81
#define I2C_WRITE_SCHEMA_LEN_ADDR 0x82
#define I2C_WRITE_DEVICE_ID_ADDR 0x83
#define I2C_WRITE_HARDWARE_ADDR 0x84

#define I2C_SET_DEFAULT 0x90
#define I2C_SET_LED_BRIGHTNESS 0x91  //not eeprom address
#define I2C_SET_MOTION 0x92          //not eeprom address
#define I2C_WRITE_BOARD_CONTEXT 0x93 //not eeprom address


#define FEATURE_INTEGER 1
#define FEATURE_BOOL 2
#define FEATURE_BYTE 3
#define FEATURE_ARRAY 4
#define FEATURE_ARRAY_LEN 5
#define FEATURE_READ_SCHEMA 100
#define FEATURE_READ_SCHEMALEN 101
#define FEATURE_READ_IDSTRING 102
#define FEATURE_READ_HARDWARE_VC 103
#define FEATURE_READ_FIRMWARE_VC 104
#define FEATURE_WRITE_FUSE 105
#define FEATURE_WRITE_SCHEMA 106
#define FEATURE_WRITE_SCHEMA_LEN 107
#define FEATURE_WRITE_HARDWARE_VC 108
#define FEATURE_WRITE_IDSTRING 109
#define FEATURE_SET_DEFAULT_VAL 110
#define FEATURE_READ_LOG_LEN 111
#define FEATURE_READ_LOG 112

#define SLAVE_ADDRESS_FLAG 0x4E
#define EEPROM_FUSE_FLAG 0x45

#define MAX_FEATURE_COUNT 20
#define MAX_WIRE_BUFFER_FLUSH_SIZE 16

class I2C_t
{
protected:
  struct Feature
  {
    uint8_t writeAddress;
    uint8_t readAddress;
    uint16_t *userDataInt;
    uint8_t *userDataByte;
    uint8_t type;
    uint8_t len;
    uint8_t defaultValByte;
    uint16_t defaultValInt;
  } Feature[MAX_FEATURE_COUNT];
  uint8_t *led_brightness_data_ptr = NULL;
  uint8_t *motion_data_ptr = NULL;
  uint8_t board_context[3];

  uint8_t addFeature(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint8_t *userData, uint8_t defaultValByte);
  uint8_t addFeature(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint16_t *userData, uint16_t defaultValInt);

  uint8_t I2C_receive(uint8_t buf);
  void I2C_request(uint8_t buf);

  uint8_t writeInternalCommand(uint8_t buf);
  void readInternalCommand(uint8_t buf);

  void setDefaultValues_cb();

  uint8_t featureCount;

public:
  uint8_t firmwareVC;
  static uint8_t schema_count;
  uint16_t countx;

  I2C_t();
  uint8_t init();
  static void stop();
  static void wireInit(uint8_t);

  static void pairingWait();
  static void pairingRequest();
  static void pairingRecieve(int);

  static void flush_wire_buffer(void);

  static void read_schema_cb();
  static void read_deviceId_cb();
  static void read_schemaLen_cb();
  static void read_hardawreVersion_cb();
  static void read_firmwareVersion_cb();
  static inline void read_board_context_cb();
  static void read_modlib_version();

  static void write_fuse_cb();
  static void write_schema_cb();
  static void write_deviceId_cb();
  static void write_schema_len_cb();
  static void write_hardawreVersion_cb();
  void setLedBrightness_cb(void);
  void setMotion_cb(void);
  void inline write_board_context_cb();

  static void requestEvent_i2c_ISR();
  static void receiveEvent_i2c_ISR(int);

  static void attach_i2c_register_cb(void (*)(void));
  static void attach_i2c_deregister_cb(void (*)(void));

  static uint8_t attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t *userData, uint8_t defaultVal);
  static uint8_t attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint16_t *userData, uint16_t defaultVal);
  static uint8_t attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint8_t *userData, uint8_t defaultVal);
  static uint8_t attachLog(uint8_t len, uint8_t *userData);
  uint8_t attachLedBrightnessData(uint8_t *data);
  uint8_t attachMotionData(uint8_t *data);
};

extern I2C_t I2C;

#endif

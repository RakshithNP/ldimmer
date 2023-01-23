#include "I2C.h"
#include <avr/eeprom.h>

volatile uint8_t i2c_get = 0;
volatile uint8_t i2c_set = 0;
int i2c_array_index;
volatile uint8_t log_index = 0;

uint8_t I2C_t ::schema_count = 0;

void I2C_t ::read_deviceId_cb()
{
  uint8_t id[23] = {0};
  eeprom_read_block(id, MEM_DEVICE_ID, 22);
  Wire.write(id, 22);
}

void I2C_t ::read_schemaLen_cb()
{
  Wire.write(eeprom_read_byte(MEM_SCHEMA_LEN_HIGH));
  Wire.write(eeprom_read_byte(MEM_SCHEMA_LEN_LOW));
  schema_count = 0;
}

void I2C_t ::read_schema_cb()
{
  uint8_t schema[17] = {0};
  int address = MEM_SCHEMA + (16 * schema_count);
  eeprom_read_block(schema, address, 16);
  Wire.write(schema, 16);
  schema_count++;
}

void I2C_t ::read_hardawreVersion_cb()
{
  Wire.write(eeprom_read_byte(MEM_HARDWARE_VC));
}

void I2C_t ::read_firmwareVersion_cb()
{
  Wire.write(I2C.firmwareVC);
}
void inline I2C_t ::read_board_context_cb()
{
  //Wire.write(board_context[0]);
  //Wire.write(board_context[1]);
  //Wire.write(board_context[2]);
  Wire.write(eeprom_read_byte(MEM_BOARD_CONTEXT_ADDRESS));
  Wire.write(eeprom_read_byte(MEM_BOARD_CONTEXT_ADDRESS + 1));
  Wire.write(eeprom_read_byte(MEM_BOARD_CONTEXT_ADDRESS + 2));
}
void I2C_t ::read_modlib_version()
{
  Wire.write(MODLIB_VERSION_MAJOR);
  Wire.write(MODLIB_VERSION_MINOR);
}
void I2C_t ::write_fuse_cb()
{
  eeprom_write_byte(MEM_EEPROM_FLAG, EEPROM_FUSE_FLAG);
}

void I2C_t ::write_hardawreVersion_cb()
{
  if (eeprom_read_byte(MEM_EEPROM_FLAG) == EEPROM_FUSE_FLAG)
  {
    eeprom_write_byte(MEM_HARDWARE_VC, Wire.read());
    eeprom_write_byte(MEM_EEPROM_FLAG, 0x00);
  }
}

void I2C_t ::write_deviceId_cb()
{
  if (eeprom_read_byte(MEM_EEPROM_FLAG) == EEPROM_FUSE_FLAG)
  {
    uint8_t id[23];
    for (uint8_t i = 0; i < 22; i++)
    {
      id[i] = Wire.read();
    }
    eeprom_write_block(id, MEM_DEVICE_ID, 22);
    eeprom_write_byte(MEM_EEPROM_FLAG, 0x00);
  }
}

void I2C_t ::write_schema_cb()
{
  if (eeprom_read_byte(MEM_EEPROM_FLAG) == EEPROM_FUSE_FLAG)
  {
    uint8_t data[16];
    for (uint8_t i = 0; i < 16; i++)
    {
      data[i] = Wire.read();
    }
    int address = MEM_SCHEMA + (16 * schema_count);
    eeprom_write_block(data, address, 16);
    schema_count++;
  }
}

void I2C_t ::write_schema_len_cb()
{
  if (eeprom_read_byte(MEM_EEPROM_FLAG) == EEPROM_FUSE_FLAG)
  {
    eeprom_write_byte(MEM_SCHEMA_LEN_HIGH, Wire.read());
    eeprom_write_byte(MEM_SCHEMA_LEN_LOW, Wire.read());
    eeprom_write_byte(MEM_EEPROM_FLAG, 0x00);
    schema_count = 0;
  }
}

void I2C_t ::setDefaultValues_cb()
{
  for (uint8_t k = 0; k < featureCount; k++)
  {
    if (Feature[k].type == FEATURE_INTEGER)
    {
      *Feature[k].userDataInt = Feature[k].defaultValInt;
    }
    else if (Feature[k].type == FEATURE_BOOL || Feature[k].type == FEATURE_BYTE)
    {
      *Feature[k].userDataByte = Feature[k].defaultValByte;
    }
  }
}
void I2C_t ::setLedBrightness_cb(void)
{
  uint8_t value = Wire.read();
  if (led_brightness_data_ptr != NULL)
  {
    if (value >= 0 && value < 255)
    {
      *led_brightness_data_ptr = value;
    }
  }
}
void I2C_t ::setMotion_cb(void)
{
  uint8_t value = Wire.read();
  if (motion_data_ptr != NULL)
  {
    if (value == 1 || value == 0)
    {
      *motion_data_ptr = value;
    }
  }
}
void inline I2C_t ::write_board_context_cb(void)
{
  for (int i = 0; i < 3; i++)
  {
    board_context[i] = Wire.read();
    EEPROM.write(MEM_BOARD_CONTEXT_ADDRESS + i, board_context[i]);
  }
}
uint8_t I2C_t ::writeInternalCommand(uint8_t buf)
{
  if (buf >= I2C_WRITE_FUSE_ADDR)
  {
    switch (buf)
    {
    case I2C_WRITE_FUSE_ADDR:
      write_fuse_cb();
      break;
    case I2C_WRITE_SCHEMA_LEN_ADDR:
      write_schema_len_cb();
      break;
    case I2C_WRITE_SCHEMA_ADDR:
      write_schema_cb();
      break;
    case I2C_WRITE_HARDWARE_ADDR:
      write_hardawreVersion_cb();
      break;
    case I2C_WRITE_DEVICE_ID_ADDR:
      write_deviceId_cb();
      break;
    case I2C_SET_DEFAULT:
      setDefaultValues_cb();
      break;
    case I2C_SET_LED_BRIGHTNESS:
      setLedBrightness_cb();
      break;
    case I2C_SET_MOTION:
      setMotion_cb();
      break;
    case I2C_WRITE_BOARD_CONTEXT:
      write_board_context_cb();
      break;
    default:
      flush_wire_buffer();
      break;
    }
    return 1;
  }
  return 0;
}

void I2C_t ::readInternalCommand(uint8_t buf)
{
  if (buf >= I2C_READ_SCHEMA_ADDR && buf < I2C_READ_MODLIB_VERSION + 1)
  {
    switch (buf)
    {
    case I2C_READ_SCHEMA_ADDR:
      read_schema_cb();
      break;
    case I2C_READ_SCHEMA_LEN_ADDR:
      read_schemaLen_cb();
      break;
    case I2C_READ_DEVICE_ID_ADDR:
      read_deviceId_cb();
      break;
    case I2C_READ_HARDWARE_VC_ADDR:
      read_hardawreVersion_cb();
      break;
    case I2C_READ_FIRMWARE_VC_ADDR:
      read_firmwareVersion_cb();
      break;
    case I2C_READ_BOARD_CONTEXT:
      read_board_context_cb();
      break;
    case I2C_READ_MODLIB_VERSION:
      read_modlib_version();
      break;
    default:;
      break;
    }
  }
}

inline uint8_t I2C_t ::I2C_receive(uint8_t buf)
{
  countx++;
  if (writeInternalCommand(buf))
  {
    return 1;
  }
  for (uint8_t k = 0; k < featureCount; k++)
  {
    if (buf == Feature[k].writeAddress)
    {
      switch (Feature[k].type)
      {
      case FEATURE_INTEGER:
        *Feature[k].userDataInt = Wire.read();
        *Feature[k].userDataInt = (*Feature[k].userDataInt << 8) | Wire.read();
        break;
      case FEATURE_BOOL:
        *Feature[k].userDataByte = Wire.read();
        break;
      case FEATURE_BYTE:
        *Feature[k].userDataByte = Wire.read();
        break;
      case FEATURE_ARRAY_LEN:
        i2c_array_index = 0;
        Wire.read();
        break;
      case FEATURE_ARRAY:
        for (uint8_t i = 0; i < 16; i++)
        {
          Feature[k].userDataByte[i2c_array_index] = Wire.read();
          i2c_array_index++;
          if (i2c_array_index > Feature[k].len)
          {
            i2c_array_index = 0;
            i = 16;
          }
        }
        break;
      default:
        flush_wire_buffer();
        break;
      }
      return 1;
    }
  }
  return 0;
}

inline void I2C_t ::I2C_request(uint8_t buf)
{
  readInternalCommand(buf);
  for (uint8_t k = 0; k < featureCount; k++)
  {
    if (buf == Feature[k].readAddress)
    {
      switch (Feature[k].type)
      {
      case FEATURE_INTEGER:
        Wire.write((*Feature[k].userDataInt >> 8) & 0xFF);
        Wire.write((*Feature[k].userDataInt) & 0xFF);
        break;
      case FEATURE_BOOL:
        Wire.write(*Feature[k].userDataByte);
        break;
      case FEATURE_BYTE:
        Wire.write(*Feature[k].userDataByte);
        break;
      case FEATURE_ARRAY_LEN:
        Wire.write((Feature[k].len >> 8) & 0xFF);
        Wire.write(Feature[k].len & 0xFF);
        i2c_array_index = 0;
        break;
      case FEATURE_ARRAY:
        if (Feature[k].len <= 16)
        {
          Wire.write(Feature[k].userDataByte, Feature[k].len);
        }
        else
        {
          for (uint8_t i = 0; i < 16; i++)
          {
            Wire.write(Feature[k].userDataByte[i2c_array_index]);
            i2c_array_index++;
            if (i2c_array_index >= Feature[k].len)
            {
              i2c_array_index = 0;
              i = 16;
            }
          }
        }
        break;
      case FEATURE_READ_LOG_LEN:
        Wire.write(Feature[k].len);
        log_index = 0;
        break;
      case FEATURE_READ_LOG:
      {
        for (uint8_t i = 0; i < 16; i++)
        {
          Wire.write(Feature[k].userDataByte[log_index]);
          log_index++;
        }
      }
      break;
      default:
        Wire.write(0);
        break;
      }
    }
  }
}

inline void I2C_t ::receiveEvent_i2c_ISR(int numberByte)
{
  if (numberByte)
  {
    i2c_set = Wire.read();
    if (I2C.I2C_receive(i2c_set) == 0)
    {
      i2c_get = i2c_set;
    }
  }
}

inline void I2C_t ::requestEvent_i2c_ISR()
{
  I2C.I2C_request(i2c_get);
}

uint8_t I2C_t ::attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t *userData, uint8_t defaultVal)
{
  uint8_t len = 0;
  switch (type)
  {
  case FEATURE_INTEGER:
    len = 2;
    break;
  case FEATURE_BOOL:
    len = 1;
    break;
  case FEATURE_BYTE:
    len = 1;
    break;
  default:
    len = 1;
    break;
  }
  return (I2C.attachEvent(readFeatureAddress, writeFeatureAddress, type, len, userData, defaultVal));
}

uint8_t I2C_t ::attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint16_t *userData, uint16_t defaultVal)
{

  return (I2C.addFeature(readFeatureAddress, writeFeatureAddress, type, FEATURE_INTEGER, userData, defaultVal));
}

uint8_t I2C_t ::attachEvent(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint8_t *userData, uint8_t defaultVal)
{
  if (type == FEATURE_READ_LOG)
  {
    I2C.addFeature(I2C_READ_LOG_LEN_ADDR, 0, FEATURE_READ_LOG_LEN, len, len, defaultVal);
  }
  return (I2C.addFeature(readFeatureAddress, writeFeatureAddress, type, len, userData, defaultVal));
}

uint8_t I2C_t ::attachLog(uint8_t len, uint8_t *userData)
{
  return (attachEvent(I2C_READ_LOG_ADDR, 0, FEATURE_READ_LOG, len, userData, 0));
}

uint8_t I2C_t ::addFeature(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint16_t *userData, uint16_t defaultVal)
{
  if (featureCount <= MAX_FEATURE_COUNT)
  {
    Feature[featureCount].readAddress = readFeatureAddress;
    Feature[featureCount].writeAddress = writeFeatureAddress;
    Feature[featureCount].type = type;
    Feature[featureCount].len = len;
    Feature[featureCount].userDataInt = userData;
    Feature[featureCount].defaultValInt = defaultVal;
    featureCount++;
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t I2C_t ::addFeature(uint8_t readFeatureAddress, uint8_t writeFeatureAddress, uint8_t type, uint8_t len, uint8_t *userData, uint8_t defaultVal)
{
  if (featureCount <= MAX_FEATURE_COUNT)
  {
    Feature[featureCount].readAddress = readFeatureAddress;
    Feature[featureCount].writeAddress = writeFeatureAddress;
    Feature[featureCount].type = type;
    Feature[featureCount].len = len;
    Feature[featureCount].userDataByte = userData;
    Feature[featureCount].defaultValByte = defaultVal;
    featureCount++;
    return 1;
  }
  else
  {
    return 0;
  }
}

uint8_t I2C_t ::attachLedBrightnessData(uint8_t *data)
{
  if (data != NULL)
  {
    led_brightness_data_ptr = data;
  }
}
uint8_t I2C_t ::attachMotionData(uint8_t *data)
{
  if (data != NULL)
  {
    motion_data_ptr = data;
  }
}

void I2C_t ::flush_wire_buffer(void)
{
  for (int i = 0; i < MAX_WIRE_BUFFER_FLUSH_SIZE; i++)
  {
    if (Wire.available())
    {
      Wire.read();
    }
    else
    {
      break;
    }
  }
  i2c_get = 0;
  i2c_set = 0;
  log_index = 0;
}

void I2C_t ::wireInit(uint8_t address)
{
  Wire.begin(address);
  TWAR = (address << 1) | 1;
  Wire.onReceive(I2C.receiveEvent_i2c_ISR);
  Wire.onRequest(I2C.requestEvent_i2c_ISR);
}

void I2C_t ::pairingRecieve(int arg)
{
  int error = 0;
  uint8_t id[23] = {0};
  eeprom_read_block(id, MEM_DEVICE_ID, 22);
  for (int i = 0; i < 22; i++)
  {
    int buf = Wire.read();
    if (id[i] == buf)
    {
      error++;
    }
  }
  if (error >= 22)
  {
    EEPROM.write(MEM_SLAVE_FLAG, SLAVE_ADDRESS_FLAG);
    EEPROM.write(MEM_SLAVE_ADDRESS, Wire.read());
  }
}

void I2C_t ::pairingRequest()
{
  uint8_t id[23] = {0};
  eeprom_read_block(id, MEM_DEVICE_ID, 22);
  Wire.write(I2C_REGISTER_SIGN_0);
  Wire.write(I2C_REGISTER_SIGN_1);
  Wire.write(I2C_REGISTER_SIGN_2);
  Wire.write(I2C_REGISTER_SIGN_3);
  Wire.write(id, 22);
  Wire.write(eeprom_read_byte(MEM_SLAVE_ADDRESS));
}

void I2C_t ::pairingWait()
{
  Wire.begin(10);
  Wire.onReceive(I2C.pairingRecieve);
  Wire.onRequest(I2C.pairingRequest);
}

void I2C_t ::stop()
{
  Wire.end();
}

uint8_t I2C_t ::init()
{
  uint8_t error = 0;
  if (eeprom_read_byte(MEM_SLAVE_FLAG) == SLAVE_ADDRESS_FLAG)
  {

    wireInit(eeprom_read_byte(MEM_SLAVE_ADDRESS));

    error = 1;
  }
  else
  {
    error = 0;
  }
  return error;
}

I2C_t ::I2C_t()
{
  firmwareVC = schema_count = 0;
  featureCount = countx = 0;
}

I2C_t I2C;

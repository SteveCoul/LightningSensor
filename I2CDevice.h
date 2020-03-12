#ifndef __I2CDevice_h__
#define __I2CDevice_h__

#include <stdint.h>

class I2CDevice {
public:
  I2CDevice( int address );
  ~I2CDevice();
  int read( uint8_t address );
  int readModifyWrite( uint8_t address, uint8_t mask, uint8_t bits );
  uint8_t readBits( uint8_t address, int shift, uint8_t mask );
  void write( uint8_t address, uint8_t data );
  void writeByte( uint8_t data );
private:
  int m_address;
};

#endif


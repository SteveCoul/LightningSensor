
#include <Arduino.h>
#include <Wire.h>
#include "I2CDevice.h"

I2CDevice::I2CDevice( int address ) : m_address( address ) { }
I2CDevice::~I2CDevice() { }

int I2CDevice::read( uint8_t address ) {
	int rc;
	Wire.beginTransmission( m_address );
	Wire.write( address );
	Wire.endTransmission( false );
	Wire.requestFrom( m_address, 1 );
	if ( Wire.available() >= 1 ) {
		rc = (uint8_t)Wire.read();
	} else {
		rc = -1;
	}
	Wire.endTransmission();
	return rc;
}

int I2CDevice::readModifyWrite( uint8_t address, uint8_t mask, uint8_t bits ) {
	int rc = read( address );
	if ( rc >= 0 ) {
		uint8_t old_data = (uint8_t)( rc & 0xFF );
		uint8_t new_data = ( old_data & ~mask ) | ( bits & mask );

		Wire.beginTransmission( m_address );
		Wire.write( address );
		Wire.write( new_data );
		if (0) {
			Serial.print("readModifyWrite @ 0x"); Serial.print( address, HEX ); Serial.print( "[mask 0x"); Serial.print(mask,HEX); Serial.print("] ");
			Serial.print("Bits 0x"); Serial.print(bits,HEX); 
			Serial.print(" changes 0x"); Serial.print(old_data,HEX); Serial.print(" to 0x"); Serial.println(new_data,HEX);
		}
		Wire.endTransmission();
	}
	return rc;
}

uint8_t I2CDevice::readBits( uint8_t address, int shift, uint8_t mask ) {
	uint8_t data = read( address );
	data >>= shift;
	data &= mask;
	return data;
}

void I2CDevice::write( uint8_t address, uint8_t data ) {
	Wire.beginTransmission( m_address );
	Wire.write( address );
	Wire.write( data );
	Wire.endTransmission();
}

void I2CDevice::writeByte( uint8_t data ) {
	Wire.beginTransmission( m_address );
	Wire.write( data );
	Wire.endTransmission();
}


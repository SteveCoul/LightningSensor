#include <stddef.h>
#include <Arduino.h>
#include "LightningSensor.h"

static int ICACHE_RAM_ATTR something_happened = 0;

void ICACHE_RAM_ATTR lightning_interrupt() {
  something_happened++;
}

LightningSensor::LightningSensor( int address, int irq_pin ) : I2CDevice( address ), m_irq_pin( irq_pin )  { 
}


void LightningSensor::listen() { something_happened = 0; 
enableInterrupt(); 
	}
int LightningSensor::poll() { if ( something_happened == 0 ) return 0; something_happened = 0; return 1; }

void LightningSensor::presetDefault() { write( 0x3C, 0x96 ); }   
  
void LightningSensor::powerUp() { clearPWD(); write( 0x3D, 0x96 ); delay(3); }

void LightningSensor::enableInterrupt() {  
  attachInterrupt(digitalPinToInterrupt(m_irq_pin), lightning_interrupt, RISING );
}

void LightningSensor::disableInterrupt() {
  detachInterrupt(digitalPinToInterrupt(m_irq_pin) );
}

void LightningSensor::powerDown() { setPWD(); }

void LightningSensor::enableDisturberDetection() { clearMASK_DIST(); }
void LightningSensor::disableDisturberDetection() { setMASK_DIST(); }

void LightningSensor::enableLCO() { readModifyWrite( 0x08, 0xE0, 0x80 ); }         /* output LC Oscillator on the IRQ pin */
void LightningSensor::enableSRCO() { readModifyWrite( 0x08, 0xE0, 0x40 ); }         /* output system Oscillator on the IRQ pin */
void LightningSensor::enableTRCO() { readModifyWrite( 0x08, 0xE0, 0x20 ); }         /* output timer Oscillator on the IRQ pin */
void LightningSensor::disableO()  { readModifyWrite( 0x08, 0xE0, 0x00 ); }
void LightningSensor::setFrequencyDivider( uint8_t val ) {   /* can actually set 16, 32, 64, 128 */
    if ( val < 24 ) val = 0;
    else if ( val < 48 ) val = 1;
    else if ( val < 96 ) val = 2;
    else val = 3;
    setLCO_FDIV( val );
}
 
int LightningSensor::getFrequencyDivider() { 
	int v = getLCO_FDIV();
	return 16 << v;
}
 
void LightningSensor::setTuningCaps( uint8_t val ) {
    if ( val > 120 ) val = 120;
    readModifyWrite( 0x08, 0x0F, val>>3 );  
}

uint8_t LightningSensor::getInterrupt() { return getINT(); };
bool LightningSensor::wasInterruptNoise( uint8_t val ) { return ( val == 1 ); }
bool LightningSensor::wasInterruptDisturbance( uint8_t val ) { return ( val == 4 ); }
bool LightningSensor::wasInterruptStrike( uint8_t val ) { return ( val == 8 ); }
bool LightningSensor::wasInterruptDistanceUpdate( uint8_t val ) { return ( val == 0 ); }

uint8_t LightningSensor::lightningDistanceInKm() { return getDISTANCE(); }

uint32_t LightningSensor::strikeEnergyRAW() {
    uint32_t rc = getS_LIG_MM();
    rc <<= 8; rc |= getS_LIG_M();
    rc <<= 8; rc |= getS_LIG_L();
    return rc;
}

void LightningSensor::setRequiredNumStrikes( int v ) {
    /* you can only actually set 1, 5, 9, 16 */
    if ( v < 3 ) v = 0;
    else if ( v < 7 ) v = 1;
    else if ( v < 13 ) v = 2;
    else v = 3;
    setMIN_NUM_LIGH( (uint8_t)v );
}

void LightningSensor::setIndoors() { setAFE_GB( 18 ); }
void LightningSensor::setOutdoors() { setAFE_GB( 14 ); }

void LightningSensor::clearStatistics() { setCL_STAT(); clearCL_STAT(); setCL_STAT(); }

/* noise floor is 0..7 see data sheet for specifics */
uint8_t LightningSensor::getNoiseFloor() { return getNF_LEV(); }
void LightningSensor::setNoiseFloor( uint8_t val ) { if ( val > 7 ) val = 7; setNF_LEV( val ); }     /* default/best is 2 */

/* 0...F more..less sensitive to disturbance. ( IE lower number is better range detection but increased false reading chances ) */
void LightningSensor::setWatchdogThreshold( uint8_t val ) { if ( val > 15 ) val = 15; setWDTH( val ); }
uint8_t LightningSensor::getWatchdogThreshold() { return getWDTH(); }

void LightningSensor::setSpikeRejection( uint8_t val ) { if ( val > 15 ) val = 15; setSREJ( val ); } /* default 2 */
uint8_t LightningSensor::getSpikeRejection() { return getSREJ(); }

uint32_t LightningSensor::measureFrequency( int which, int divider ) {
	uint32_t count;

	setFrequencyDivider( divider );

	if ( which == 0 ) enableLCO();
	else if ( which == 1 ) enableSRCO();
	else enableTRCO();

	delay(2);
 	enableInterrupt();
	something_happened = 0;
	delay(1000);
	disableInterrupt();
	count = something_happened * divider;
	disableO();
	return count;
}

bool LightningSensor::calibrate() {
    Serial.println("Calibrating....");
    int target = 500000, currentcount = 0, bestdiff = 0x7FFFFFFF, currdiff = 0;
    uint8_t bestTune = 0, currTune = 0;
    for (currTune = 0; currTune <= 0x0F; currTune++) {
      readModifyWrite( 0x08, 0x0F, currTune );  

	  delay(2);
      currentcount = measureFrequency( 0, 32 );

      currdiff = target - currentcount;
      if (currdiff < 0) currdiff = -currdiff;
      Serial.print("Tune "); Serial.print(currTune); Serial.print(" got count of "); Serial.print( currentcount ); Serial.print(" diff "); Serial.println( currdiff );
      if (bestdiff > currdiff) {
        bestdiff = currdiff;
        bestTune = currTune;
      }
    }
    readModifyWrite( 0x08, 0x0F, bestTune );  
   
    Serial.print(F("bestTune = "));
    Serial.println(bestTune);
    Serial.print(F("Difference ="));
    Serial.println(bestdiff);

    delay(100);
    calibrateRCO();

    return bestdiff > 17500 ? false : true;
}

void LightningSensor::dump() {
    Serial.println("");
    Serial.print("AFE_GB "); Serial.println( getAFE_GB(), HEX );  
    Serial.print("PWD "); Serial.println( getPWD(), HEX );  
    Serial.print("NF_LEV " ); Serial.println( getNF_LEV(), HEX );
    Serial.print("WDTH "); Serial.println( getWDTH(), HEX );
    Serial.print("CL_STAT ");  Serial.println( getCL_STAT(), HEX );
    Serial.print("MIN_NUM_LIGH ");  Serial.println( getMIN_NUM_LIGH(), HEX );
    Serial.print("SREJ ");  Serial.println( getSREJ(), HEX );
    Serial.print("LCO_FDIV ");  Serial.println( getLCO_FDIV(), HEX );
    Serial.print("MASK_DIST ");  Serial.println( getMASK_DIST(), HEX );
    Serial.print("INT ");  Serial.println( getINT(), HEX );
    Serial.print("S_LIG_L ");  Serial.println( getS_LIG_L(), HEX );
    Serial.print("S_LIG_M ");  Serial.println( getS_LIG_M(), HEX );
    Serial.print("S_LIG_MM ");  Serial.println( getS_LIG_MM(), HEX );
    Serial.print("DISTANCE ");  Serial.println( getDISTANCE(), HEX );
    Serial.print("DISP_LCO ");  Serial.println( getDISP_LCO(), HEX );
    Serial.print("DISP_SRCO ");  Serial.println( getDISP_SRCO(), HEX );
    Serial.print("DISP_TRCO ");  Serial.println( getDISP_TRCO(), HEX );
    Serial.print("TUN_CAP ");  Serial.println( getTUN_CAP(), HEX );
    
	Serial.print( "LCO " ); Serial.print( measureFrequency( 0, 128 ) ); Serial.println(" Hz, want 500,000" ); 
//	Serial.print( "SRCO " ); Serial.print( measureFrequency( 1, 128 ) ); Serial.println(" Hz, want 1,100,000" );
//	Serial.print( "TRCO " ); Serial.print( measureFrequency( 2, 128 ) ); Serial.println(" Hz, want 32,768" );
}

void LightningSensor::calibrateRCO() { write( 0x3D, 0x96 ); enableTRCO(); delay(5); disableO(); }

uint8_t LightningSensor::getPWD() { return readBits( 0x00, 0, 0x01 ); }
void LightningSensor::clearPWD()  { (void)readModifyWrite( 0x00, 0x01, 0x00 ); }
void LightningSensor::setPWD()    { (void)readModifyWrite( 0x00, 0x01, 0x01 ); }
  
uint8_t LightningSensor::getAFE_GB() { return readBits( 0x00, 1, 0x01F ); }        /* defaults of 0x12 for indoor and 0x0E for outdoor */
void LightningSensor::setAFE_GB( uint8_t val ) { (void)readModifyWrite( 0x00, 0x3E, (val&0x1F)<<1 ); }
  
void LightningSensor::setDISP_SRC0() { (void)readModifyWrite( 0x08, 0x20, 0x20 ); }
void LightningSensor::clearDISP_SRC0() { (void)readModifyWrite( 0x08, 0x20, 0x00 ); }
  
uint8_t LightningSensor::getNF_LEV() { return readBits( 0x01, 4, 0x07 ); }
void LightningSensor::setNF_LEV( uint8_t val )    { readModifyWrite( 0x01, 0x70, (val&0x07) <<4 ); }
  
uint8_t LightningSensor::getWDTH() { return readBits( 0x01, 0, 0x0F ); }
void LightningSensor::setWDTH( uint8_t val ) { readModifyWrite( 0x01, 0x0F, val & 0x0F ); }
  
uint8_t LightningSensor::getCL_STAT() { return readBits( 0x02, 6, 0x01 ); }
void LightningSensor::clearCL_STAT()  { (void)readModifyWrite( 0x02, 0x40, 0x00 ); }
void LightningSensor::setCL_STAT()  { (void)readModifyWrite( 0x02, 0x40, 0x40 ); }
  
uint8_t LightningSensor::getMIN_NUM_LIGH() { return readBits( 0x02, 4, 0x03 ); }
void LightningSensor::setMIN_NUM_LIGH( uint8_t val ) { (void)readModifyWrite( 0x02, 0x30, (val&3)<<4 ); }
  
uint8_t LightningSensor::getSREJ() { return readBits( 0x02, 0, 0x0F ); }
void LightningSensor::setSREJ( uint8_t val ) { readModifyWrite( 0x02, 0x0F, val & 0x0F ); }
  
uint8_t LightningSensor::getLCO_FDIV() { return readBits( 0x03, 6, 0x03 ); }
void LightningSensor::setLCO_FDIV( uint8_t val ) { readModifyWrite( 0x03, 0xC0, (val&3)<<6 ); }
  
uint8_t LightningSensor::getMASK_DIST() { return readBits( 0x03, 5, 0x01 ); }
void LightningSensor::clearMASK_DIST()  { (void)readModifyWrite( 0x03, 0x20, 0x00 ); }
void LightningSensor::setMASK_DIST()    { (void)readModifyWrite( 0x03, 0x20, 0x20 ); }
  
uint8_t LightningSensor::getINT() { return readBits( 0x03, 0, 0x0F ); }
uint8_t LightningSensor::getS_LIG_L() { return readBits( 0x04, 0, 0xFF ); }
uint8_t LightningSensor::getS_LIG_M() { return readBits( 0x05, 0, 0xFF ); }
uint8_t LightningSensor::getS_LIG_MM() { return readBits( 0x06, 0, 0x1F ); }
uint8_t LightningSensor::getDISTANCE() { return readBits( 0x07, 0, 0x3F ); }
  
uint8_t LightningSensor::getDISP_LCO() { return readBits( 0x08, 7,  0x01 ); }
void LightningSensor::setDISP_LCO(uint8_t val ) { readModifyWrite( 0x08, 0x80, val ? 0x80 : 0 ); }
uint8_t LightningSensor::getDISP_SRCO() { return readBits( 0x08, 6, 0x01 ); }
uint8_t LightningSensor::getDISP_TRCO() { return readBits( 0x08, 5, 0x01 ); }
  
uint8_t LightningSensor::getTUN_CAP() { return readBits( 0x08, 0, 0x0F ); }
void LightningSensor::setTUN_CAP( uint8_t val ) { readModifyWrite( 0x08, 0x0F, val>>3 );  }



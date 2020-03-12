#ifndef __LightningSensor_h__
#define __LightningSensor_h__

#include "I2CDevice.h"

class LightningSensor : public I2CDevice {
public:
  LightningSensor( int address, int irqpin );
  void presetDefault();
  void powerUp();
  void powerDown();
  void enableDisturberDetection();
  void disableDisturberDetection();
  void enableLCO();
  void enableSRCO();
  void enableTRCO();
  void disableO();
  void setFrequencyDivider( uint8_t val );
  void setTuningCaps( uint8_t val );
  uint8_t getInterrupt();
  bool wasInterruptNoise( uint8_t val );
  bool wasInterruptDisturbance( uint8_t val );
  bool wasInterruptStrike( uint8_t val );
  bool wasInterruptDistanceUpdate( uint8_t val );
  uint8_t lightningDistanceInKm();
  uint32_t strikeEnergyRAW();
  void setRequiredNumStrikes( int v );
  void setIndoors();
  void setOutdoors();
  void clearStatistics();
  uint8_t getNoiseFloor();
  void setNoiseFloor( uint8_t val );
  void setWatchdogThreshold( uint8_t val );
  uint8_t getWatchdogThreshold();
  void setSpikeRejection( uint8_t val );
  uint8_t getSpikeRejection();
  bool calibrate();
  void dump();
private:
  void calibrateRCO();
  uint8_t getPWD();
  void clearPWD();
  void setPWD();
  uint8_t getAFE_GB();
  void setAFE_GB( uint8_t val );
  void setDISP_SRC0();
  void clearDISP_SRC0();
  uint8_t getNF_LEV();
  void setNF_LEV( uint8_t val );
  uint8_t getWDTH();
  void setWDTH( uint8_t val );
  uint8_t getCL_STAT();
  void clearCL_STAT();
  void setCL_STAT();
  uint8_t getMIN_NUM_LIGH();
  void setMIN_NUM_LIGH( uint8_t val );
  uint8_t getSREJ();
  void setSREJ( uint8_t val );
  uint8_t getLCO_FDIV();
  void setLCO_FDIV( uint8_t val );
  uint8_t getMASK_DIST();
  void clearMASK_DIST();
  void setMASK_DIST();
  uint8_t getINT();
  uint8_t getS_LIG_L();
  uint8_t getS_LIG_M();
  uint8_t getS_LIG_MM();
  uint8_t getDISTANCE();
  uint8_t getDISP_LCO();
  void setDISP_LCO(uint8_t val );
  uint8_t getDISP_SRCO();
  uint8_t getDISP_TRCO();
  uint8_t getTUN_CAP();
  void setTUN_CAP( uint8_t val );
private:
  int m_irq_pin;
};

#endif


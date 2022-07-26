#include "SmartBracelet.h"


configuration SmartBraceletAppC {}

implementation {
  components MainC, SmartBraceletC as App;
  
  components new AMSenderC(AM_RADIO_TYPE);
  components new AMReceiverC(AM_RADIO_TYPE);
  components ActiveMessageC as RadioAM;
  
  components new TimerMilliC() as TimerPairing;
  components new TimerMilliC() as TimerInfo;
  components new TimerMilliC() as TimerAlert;

  components new PositionSensorC();
  
  //components SerialActiveMessageC as AMSerial;
  
  // Boot interface
  App.Boot -> MainC.Boot;
  
  // Radio interface
  App.AMSend -> AMSenderC;
  App.Receive -> AMReceiverC;
  App.AMControl -> RadioAM;
  
  App.Packet -> AMSenderC;
  App.AMPacket -> AMSenderC;
  App.PacketAcknowledgements -> RadioAM;

  // Timers
  App.TimerPairing -> TimerPairing;
  App.TimerInfo -> TimerInfo;
  App.TimerAlert -> TimerAlert;
  

  App.PositionSensor -> PositionSensorC;
  

  // Serial port
  //App.SerialControl -> AMSerial;
  //App.SerialAMSend -> AMSerial.AMSend[AM_MY_SERIAL_MSG];
 // App.SerialPacket -> AMSerial;
  
}



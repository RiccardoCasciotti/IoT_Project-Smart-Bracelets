#include "Timer.h"
#include "SmartBracelet.h"
#include <stdio.h>

module SmartBraceletC @safe() {
  uses {
    interface Boot;
    
    interface AMSend;
    interface Receive;
    interface SplitControl as AMControl;
    interface Packet;
    interface AMPacket;
    interface PacketAcknowledgements;
    
    interface Timer<TMilli> as TimerPairing;
    interface Timer<TMilli> as TimerInfo;
    interface Timer<TMilli> as TimerAlert;
    
    interface Read<sensor_status> as PositionSensor;
  
  }
}

implementation {
  
 // Radio control
  bool busy = FALSE;
  uint16_t counter = 0;
  message_t packet;
  am_addr_t address_coupled_device;
  uint8_t attempt = 0;
  
  // Current mode
  uint8_t mode = 0;
  
  // Sensors
  bool sensors_read_completed = FALSE;
  
  sensor_status status;
  sensor_status last_status;
  

  void send_confirmation();
  void send_position();
  
  // Program start
  event void Boot.booted() {
    call AMControl.start();
  }

  // called when radio is ready
  event void AMControl.startDone(error_t err) {
    if (err == SUCCESS) {
      dbg("Radio", "Radio device ready\n");
      dbg("Pairing", "Pairing mode started\n");
      
      // Start pairing mode
      call TimerPairing.startPeriodic(250);
    } else {
      call AMControl.start();
    }
  }
  
  event void AMControl.stopDone(error_t err) {}
  

  event void TimerPairing.fired() {
    
  }
  
  // TimerInfo fired
  event void TimerInfo.fired() {
    dbg("TimerInfo", "TimerInfo: timer fired at time %s\n", sim_time_string());
    call PositionSensor.read();
  }

  // TimerAlert fired
  event void TimerAlert.fired() {
    dbg("TimerAlert", "TimerAlert: timer fired at time %s\n", sim_time_string());
    dbg("Info", "ALERT: MISSING");
    dbg("Info","Last known location: %hhu, Y: %hhu\n", last_status.X, last_status.Y);

    //send to serial here

  }

 
  
  event void AMSend.sendDone(message_t* bufPtr, error_t error) {
    
  }
  

   
  event message_t* Receive.receive(message_t* bufPtr, void* payload, uint8_t len) {
    
  }

 
 



  // Send confirmation in mode 1
  void send_confirmation(){
   
  }
  
   event void PositionSensor.readDone(error_t result, sensor_status current_read) {
	if( result == SUCCESS ){ //check that the reading went well
        status = current_read;
        sensors_read_completed = TRUE;
        send_position();
        sensors_read_completed = FALSE; // reset the sensor reading routine var
    } 
  
  }
  
  // Send INFO message from child's bracelet
  void send_position(){
    
    // Check that the instance is a child ----?  Use TOS_ID

    //I need to make the sensor reading and gather the data
    if(sensors_read_completed){
      bool msg_state = FALSE;
      // Prepare the info message and fill it with the sensor data
      smartB_msg_t* info_message = (smartB_msg_t*)call Packet.getPayload(&packet, sizeof(smartB_msg_t));
      info_message->msg_type = 2;
      info_message->msg_id = counter++;
      info_message->X = status.X;
      info_message->Y = status.Y;
      
      if( strcpy(info_message->data, status.status) == info_message->data)
        msg_state = TRUE;

      /*
      // The child requires an ACK for the info mesage sent to the parent
      call PacketAcknowledgements.requestAck( &packet );
      */

     // Send the info message to the parent bracelet using the unique address.
     if (!busy && call AMSend.send(address_coupled_device, &packet, sizeof(smartB_msg_t)) == SUCCESS ) {
          dbg("Radio", "Radio: sending INFO packet to node %hhus\n", address_coupled_device);	
          busy = TRUE;
        }

    }
  }
  
}


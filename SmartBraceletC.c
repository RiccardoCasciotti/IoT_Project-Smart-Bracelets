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

/* Convention: 

  TOS_ID % 2 = 0 ---> PARENT
  TOS_ID % 2 != 0 ---> CHILD

*/


implementation {
  
 // Radio control
  bool busy = FALSE;
  bool paired = FALSE; // to check if the parent is paired to a child
  uint16_t counter = 0;
  message_t packet;
  am_addr_t parent_key;
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
  

   
  event message_t* Receive.receive(message_t* rec_packet, void* payload, uint8_t len) {
    // The receive is organized by use case and is going to call other functions depending 
    // on the message received ( to have a clearer code ).
    smartB_msg_t* message = (smartB_msg_t*)payload;
    // Print data of the received packet
	  dbg("Receive","Message received from bracelet %hhu at time %s\n", call AMPacket.source( rec_packet ), sim_time_string());
	  dbg("Received_package","Payload: type: %hu, msg_id: %hhu, data: %s\n", message->msg_type, message->msg_id, message->data);
    
    //Received a pairing message

    //Received an info message
    if(message->msg_type == 2 ){

      // Check if the instance is a parent, if the parent is paired and if the message comes from
      // the paired child
      if( TOS_NODE_ID%2 == 0 && paired && call AMPacket.source( rec_packet ) == parent_key){
        last_status->X = message->X;
        last_status->Y = message->Y;


        dbg("Received_package","INFO message received\n");
        dbg("Info", "Position X: %hhu, Y: %hhu\n", message->X, message->Y);
        dbg("Info", "Sensor status: %s\n", message->data);
        
        // Check if the status is FALLING and signal an alarm
        if (strcmp(message->data, "FALLING") == 0){
            dbg("Info", "ALERT: FALLING!\n");
 	          //send to serial here
        }


        // Start the alert timer, if it finishes because it never gets caled back again then 
        // the child is missing
        call TimerAlert.startOneShot(60000);

      }
    }
    
    
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
     if (!busy && call AMSend.send(parent_key, &packet, sizeof(smartB_msg_t)) == SUCCESS ) {
          dbg("Radio", "Radio: sending INFO packet to node %hhus\n", parent_key);	
          busy = TRUE;
        }

    }
  }
  
}


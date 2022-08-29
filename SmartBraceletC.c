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

  TOS_ID % 2 == 0 ---> PARENT
  TOS_ID % 2 == 1 ---> CHILD

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

  /*

  mode == 0 pairing
  mode == 1 confirmation pairing
  mode == 2 operation
  mode == 3 alert

  */
  
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
    counter++;
    dbg("TimerPairing", "TimerPairing: timer fired at time %s\n", sim_time_string());
    if (!busy) {
      smartB_msg_t* smartB_pairing_message = (smartB_msg_t*)call Packet.getPayload(&packet, sizeof(smartB_msg_t));
      
      // Fill payload
      smartB_pairing_message->msg_type = 0; // 0 for pairing phase
      smartB_pairing_message->msg_id = counter;

      //The node ID is divided by 2 so every 2 nodes will be the same number (0/2=0 and 1/2=0)
      //we get the same key for every 2 nodes: parent and child
      strcpy(smartB_pairing_message->data, RANDOM_KEY[TOS_NODE_ID/2]);
      
      if (call AMSend.send(AM_BROADCAST_ADDR, &packet, sizeof(smartB_msg_t)) == SUCCESS) {
	      dbg("Radio", "Radio: sending pairing packet, key=%s\n", RANDOM_KEY[TOS_NODE_ID/2]);	
	      busy = TRUE;
      }
    }
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

 
  
  event void AMSend.sendDone(message_t* rec_packet, error_t error) {
    if (&packet == rec_packet && error == SUCCESS) {
      dbg("Radio_sent", "Packet sent\n");
      busy = FALSE;
      
      if (mode == 1 && call PacketAcknowledgements.wasAcked(rec_packet) ){
        // mode == 1 and ack received
        mode = 2; // Pairing phase 1 completed
        dbg("Radio_ack", "Pairing ack received at time %s\n", sim_time_string());
        dbg("Pairing","Pairing phase 1 completed for node: %hhu\n\n", address_coupled_device);
        
        // Start operational phase
        if (TOS_NODE_ID % 2 == 0){
          // Parent bracelet
          dbg("OperationalMode","Parent bracelet\n");
          //call SerialControl.start();
          call Timer60.startOneShot(60000);
        } else {
          // Child bracelet
          dbg("OperationalMode","Child bracelet\n");
          call Timer10.startPeriodic(10000);
        }
      
      } else if (mode == 1){
        // Mode == 1 but ack not received
        dbg("Radio_ack", "Pairing ack not received at time %s\n", sim_time_string());
        send_confirmation(); // Send confirmation again
      
      } else if (mode == 2 && call PacketAcknowledgements.wasAcked(rec_packet)){
        // Mode == 2 and ack received
        dbg("Radio_ack", "INFO ack received at time %s\n", sim_time_string());
        attempt = 0;
        
      } else if (mode == 2){
        // Mode == 2 and ack not received
        dbg("Radio_ack", "INFO ack not received at time %s\n", sim_time_string());
        send_info_message();
      }
        
    }

  }
  

   
  event message_t* Receive.receive(message_t* rec_packet, void* payload, uint8_t len) {
    // The receive is organized by use case and is going to call other functions depending 
    // on the message received ( to have a clearer code ).
    smartB_msg_t* message = (smartB_msg_t*)payload;
    // Print data of the received packet
	  dbg("Receive","Message received from bracelet %hhu at time %s\n", call AMPacket.source( rec_packet ), sim_time_string());
	  dbg("Received_package","Payload: type: %hu, msg_id: %hhu, data: %s\n", message->msg_type, message->msg_id, message->data);
    
    //Received a pairing message

    // checknig that is a broadcast message, that the mode is 0 (pairing phase)  
    // and that the key is the same
    if (call AMPacket.destination( rec_packet ) == AM_BROADCAST_ADDR && phase == 0 && strcmp(message->data, RANDOM_KEY[TOS_NODE_ID/2]) == 0){
      
      parent_key = call AMPacket.source( bufPtr );
      mode = 1; // 1 for confirmation of pairing phase
      dbg("Radio_pack","Message for pairing phase 0 received. Address: %hhu\n", parent_key);
      send_confirmation();
    
    } else if (call AMPacket.destination( bufPtr ) == TOS_NODE_ID && mess->msg_type == 1) {
      // Enters if the packet is for this destination and if the msg_type == 1
      dbg("Radio_pack","Message for pairing phase 1 received\n");
      call TimerPairing.stop();

    //Received an info message
    else if(call AMPacket.destination( bufPtr ) == TOS_NODE_ID && message->msg_type == 2){

      // Check if the instance is a parent, if the parent is paired and if the message comes from
      // the paired child
      if( TOS_NODE_ID%2 == 0 && paired && call AMPacket.source( rec_packet ) == parent_key){
        last_status->X = message->X;
        last_status->Y = message->Y;

        dbg("Received_package","INFO message received\n");
        dbg("Info", "Position X: %hhu, Y: %hhu\n", message->X, message->Y);
        dbg("Info", "Sensor status: %s\n", message->data);
        call TimerAlert.start();

        // Check if the status is FALLING and signal an alarm
        if (message->data == 3){
            dbg("Info", "ALERT: FALLING!\n");
 	          //send to serial here
    }

        // Start the alert timer, if it finishes because it never gets caled back again then 
        // the child is missing
        call TimerAlert.startOneShot(60000);

      }
  }
   

 
 



  // Send confirmation in mode 1
  void send_confirmation(){

   counter++;
    if (!busy) {
      smartB_msg_t* sb_pairing_message = (smartB_msg_t*)call Packet.getPayload(&packet, sizeof(smartB_msg_t));
      
      // Fill payload
      sb_pairing_message->msg_type = 1; // 1 for confirmation of pairing phase
      sb_pairing_message->msg_id = counter;
      
      strcpy(sb_pairing_message->data, RANDOM_KEY[TOS_NODE_ID/2]);
      
      /* Require ack
      call PacketAcknowledgements.requestAck( &packet );
      */
      
      if (call AMSend.send(parent_key, &packet, sizeof(smartB_msg_t)) == SUCCESS) {
        dbg("Radio", "Radio: sending pairing confirmation to node %hhu\n", parent_key);	
        busy = TRUE;
      }
    }
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
      info_message->msg_id = ++counter;
      info_message->X = status.X;
      info_message->Y = status.Y;
      
      if( strcpy(info_message->data, status.status) == info_message->data )
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


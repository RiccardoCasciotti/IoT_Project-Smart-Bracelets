#ifndef SMARTBRACELET_H
#define SMARTBRACELET_H

// Message struct
typedef nx_struct smartB_msg_t {
  	nx_uint8_t msg_type;
  	nx_uint16_t msg_id;
  	nx_uint8_t data[20];
  	nx_uint16_t X;
  	nx_uint16_t Y;
} smartB_msg_t;

typedef struct sensorStatus {
  uint8_t status[10];
  uint16_t X;
  uint16_t Y;
} sensor_status;

/*
Convention: status is an integer

0 is standing 
1 is walking
2 is running 
3 is falling 

to solve the string compare types bug

*/


// Constants
enum {
  AM_RADIO_TYPE = 6,
  
};

#endif

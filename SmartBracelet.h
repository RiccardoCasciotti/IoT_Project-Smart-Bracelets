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


// Constants
enum {
  AM_RADIO_TYPE = 6,
  
};

// Pre-loaded random keys
#define FOREACH_KEY(KEY) \
        KEY(QupLLo2877mHWlAsSHvY) \
        KEY(wzqpKCsbn9R0v9OcFgdS) \
        KEY(eYQomjCfG6y6Wq5P65QV) \
        KEY(zbmn3V2Licz76QIYCnZH) \
        KEY(QJIpr3uqMKhFOrgSya84) \
        KEY(N1Rpe4XJvetQAiYrbXoL) \
        KEY(aCsH4cRnnQ3LJFaC0Dfy) \
        
#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,
enum KEY_ENUM {
    FOREACH_KEY(GENERATE_ENUM)
};
static const char *PRESTORED_KEY[] = {
    FOREACH_KEY(GENERATE_STRING)
};


#endif

#ifndef __OBD_DATA_H__
#define __OBD_DATA_H__

#include <stdint.h>

typedef struct {
    uint16_t rpm;
    uint8_t speed;
    int8_t coolant;
    int8_t intake;
    uint8_t throttle;
    float maf;
    uint8_t map;
    float lambdaV;
    uint32_t runtime;
    float fuelRate;

    uint16_t distMilOn;
    uint8_t fuelSys1Status;
    uint8_t evapPurge;
    uint8_t warmups;
    uint16_t distSinceCLR;
    uint8_t baro;
} OBD_Data_t;

extern OBD_Data_t obdData;

#endif

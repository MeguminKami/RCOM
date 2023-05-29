#include "linklayer.h"

#define F 0x5C
#define SEND 0x01
#define RECV 0x03 
#define SET 0x02
#define RR0 0x01
#define RR1 0x21
#define REJ0 0x05
#define REJ1 0x25
#define DISC 0x0B
#define UA 0x07
#define END 0x5D
#define SC 0x7C
#define SD 0x7D

typedef enum
{
    START,
    FRCV,
    ARCV,
    REJ,
    CRCV,
    BCCOK,
    STOPED,
    WAIT_1,
    WAIT_2,
}estado;

// update da maquina de estados
estado updateState(estado currentState, unsigned char byte, int currentRole);
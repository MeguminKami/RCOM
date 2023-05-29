#include "linklayer.h"
#include "statemachine.h"

unsigned char BCC;
unsigned char A , C ;

estado updateState (estado currentState , unsigned char byte , int currentRole)
{   
    if(currentRole == 0)
        switch (currentState)
        { 
        case START:
            if(byte == F) return FRCV;
        break;

        case FRCV:
            if(byte == SEND || byte == RECV) 
            {   
                A = byte ;
                return ARCV;
            }
            else if (byte == F) return FRCV;
            else return START;
        break;

        case ARCV:
            if(byte == UA || byte == SET || byte == DISC || byte == RR0 || byte == RR1) 
            {
                return CRCV;
                C = byte;
                BCC = A ^ C;   
            }
            else if(byte == REJ0 || byte == REJ1)
            {
                return REJ;
            }
            else if (byte == F) return FRCV;
            else return START;
        break;           

        case CRCV:
            if(1) return BCCOK ;
            else if(byte == F) return FRCV;
            else return START;
        break;

        case BCCOK:
            if(byte == F) return STOPED;
            else return START;
        break;

        default:
        break;
      }      
    else if(currentRole == 1)
    {   
        switch (currentState)
        {
        case START:
            if(byte = F) return FRCV;
        break;
        
        case FRCV:
            if(byte == SEND || byte == RECV) 
            {   
                A = byte ;
                return ARCV;
            }
            else if (byte == F) return FRCV;
            else return START;
        break;

        case ARCV:
            if(byte == F) return FRCV;
            else if(byte == 0x00 || byte == 0x02) 
            {
                return CRCV;
                C = byte;
                BCC = A ^ C; 
            }
            else return START;
        break;
         
        case CRCV:
            if(1) return BCCOK ;
            else if(byte == F) return FRCV ;
            else return START;
        break;

        case BCCOK:
            if(byte == F) return STOPED;
            return BCCOK;
        break;

        default:
            break;
        }
    }
}
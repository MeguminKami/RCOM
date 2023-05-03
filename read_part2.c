/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "linklayer.h"
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

typedef enum
{
    START,
    FRCV,
    ARCV,
    CRCV,
    BCCOK,
    STOPED,

} SETMSG ;

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS4", argv[1])!=0) &&
          (strcmp("/dev/ttyS11", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if (tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    /*
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a
    leitura do(s) próximo(s) caracter(es)
    */


    tcflush(fd, TCIOFLUSH);

    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");
    
    unsigned char UA_FRAME[5];
    unsigned char F = {0x5C};
    unsigned char SEND = {0x01};
    unsigned char RECV = {0x03}; 
    unsigned char SET = {0x02};
    unsigned char DISC = {0x0B};
    unsigned char UA = {0x07};
    unsigned char DADOS = {0x5D};
    unsigned char BCC ;
    unsigned char A ;
    unsigned char C ;
    SETMSG ESTADO = START ;
  
    
    while(STOP == FALSE) 
    {   
        res = read(fd,buf,1);

      switch (ESTADO)
      { 
        case START:
            if(buf[0] == F) ESTADO = FRCV;
        break;

        case FRCV:
            if(buf[0] == SEND) 
            {   
                A = buf[0] ;
                ESTADO = ARCV;
            }
            else if (buf[0] == F) ESTADO = FRCV;
            else ESTADO == START;
            
        break;

        case ARCV:
            if(buf[0] == SET) 
            {
                ESTADO = CRCV;
                C = buf[0];
                BCC = A ^ C;   
            }
            else if (buf[0] == F) ESTADO = FRCV;
            else ESTADO = START;
        break;

        case CRCV:
            if(buf[0] == BCC) ESTADO = BCCOK ;
            else if(buf[0] == F) ESTADO = FRCV;
            else ESTADO = START;
        break;

        case BCCOK:
            if(buf[0] == F) ESTADO = STOPED;
            else ESTADO = START;
        break;

        case STOPED:
            STOP = TRUE ;
        break;

        default:
        break;
      }      

    }
    if(ESTADO == STOPED) printf("RCV & SENT\n");
    else printf("ERROR\n");
    
    BCC = SEND ^ UA ;
    UA_FRAME[0] = F;
    UA_FRAME[1] = SEND;
    UA_FRAME[2] = UA;
    UA_FRAME[3] = BCC;
    UA_FRAME[4] = F;
    buf[0] = '\0';

    // UA FRAME
    for(int i = 0 ; i < 6 ; i++) buf[i] = UA_FRAME[i];
    res = write(fd,buf,255);
    
    
    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}

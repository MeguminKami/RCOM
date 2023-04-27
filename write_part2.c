/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include "linklayer.h"
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS11"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    char buf[255];
    int i, sum = 0, speed = 0;

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS10", argv[1])!=0) &&
          (strcmp("/dev/ttyS0", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }


    /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */


    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
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

    char* MSG[255] ;
    strcpy(MSG,"up201906465@fe.up.pt");
    unsigned int FRAME_SIZE = 6;
    unsigned char frame[FRAME_SIZE];
    unsigned char F = {0x5C};
    unsigned char SEND = {0x01};
    unsigned char RECV = {0x03}; 
    unsigned char SET = {0x02};
    unsigned char DISC = {0x0B};
    unsigned char UA = {0x07};
    unsigned char DADOS = {0x5D};
    frame[0] = F;
    frame[1] = SEND;
    frame[2] = SET;
    frame[3] = DADOS;
    frame[4] = F;
    
    int hex_num = (int)frame[0];
    sprintf(buf,"%02x",hex_num);
    buf[strlen(buf)] = '\0';
    res = write(buf,fd,255);
    printf("Sending %s of %d bits.\n",buf,res);
    /*
    printf("\nSending:");
    for(int i = 0 ; i < FRAME_SIZE ; i++)
    {
        if(frame[i] == DADOS)
        {
            write(fd,frame[i],255);
            write(fd,MSG,255);
            printf("%x ",frame[i]);
            printf("%s ",MSG);
            continue;
        }
        printf("%x ",frame[i]);
        write(fd,frame[i],255);   
    }
    printf("\n");
    */
    

    /*
    O ciclo FOR e as instruções seguintes devem ser alterados de modo a respeitar
    o indicado no guião
    */

    sleep(1);

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 0;
}

#include "linklayer.h"
#include "statemachine.h"

#define BUF_SIZE 255 
linkLayer srcParameters ;
estado openState ;
estado writeState ;
estado readState ;
estado closeState = START ;
struct termios oldtio;
struct termios newtio;
int alarme_on = 0;
int alarme_count = 0 ;
int Ns = 0;
int Nr = 0;
int fd;
int res;

void atende()
{
    alarme_on = 0;
    alarme_count++;
}

int llopen(linkLayer connectionParameters)
{
    unsigned char buf[BUF_SIZE];
    srcParameters = connectionParameters;

    fd = open(srcParameters.serialPort,O_RDWR | O_NOCTTY);
    if(fd < 0)
    {
        perror("Error");
        exit(-1);
    }

    if(tcgetattr(fd , &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    
    bzero(&newtio,sizeof(newtio));
    newtio.c_cflag = srcParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; 
    newtio.c_cc[VMIN] = 1;  
    
    tcflush(fd, TCIOFLUSH);
    if (tcsetattr(fd,TCSANOW,&newtio) == -1) 
    {
        return(-1);
    }

    unsigned char SET_FRAME[5] = {0};
    SET_FRAME[0] = F;
    SET_FRAME[1] = SEND;
    SET_FRAME[2] = SET;
    SET_FRAME[3] = SET_FRAME[1] ^ SET_FRAME[2];
    SET_FRAME[4] = F;

    unsigned char UA_FRAME[5] = {0};
    UA_FRAME[0] = F;
    UA_FRAME[1] = SEND;
    UA_FRAME[2] = UA;
    UA_FRAME[3] = UA_FRAME[1] ^ UA_FRAME[2];
    UA_FRAME[4] = F;


    openState = START ;
    switch (srcParameters.role)
    {
        case TRANSMITTER: // Transmitter  

            (void)signal(SIGALRM, atende); 
            
            while(alarme_count < srcParameters.numTries)
            {
                if(!alarme_on)
                {
                    openState = START;
                    write(fd,SET_FRAME,5);
                    alarm(3);
                    alarme_on = 1 ;
                }

            }
                read(fd,buf,1);
                openState = updateState(openState,buf[0],0);
                if(openState == STOPED) break;

        break;

    case RECEIVER: // Receiver
        while(1)
        {
            read(fd,buf,1);
            openState = updateState(openState,buf[0],0);
            if(openState == STOPED) break;
             
        }
        write(fd,UA_FRAME,5);
    break;
    
    default:
    break;
    }

    if(alarme_count > srcParameters.numTries) 
    {   
        printf("Failed to establish a connection\n");
        return -1;
    }
    alarme_count = 0;
    alarme_on = 0;
}

int llwrite(char *buf, int bufSize)
{   
    writeState = START ;

    // Data will be the full stuffed frame
    char data[MAX_PAYLOAD_SIZE*2];
    // Since we are stuffing payloadsize needs to be bigger 
    // Double size should be more then enough

    int dataSize = 4; // Starts with 4 because of the header


    data[0] = F;
    data[1] = SEND;
    data[2] = (Ns == 0) ? 0x00 : 0x02; // S = N(s)
    data[3] = data[1] ^ data[2]; // BCC

    int BCC2 = 0; // BCC2 xor of all bytes in the buf
    printf("\nBUF");
    for(int i = 0; i < bufSize; i++) 
    { 
        BCC2 ^= buf[i];
        printf("Sent: 0x%02x.\n",buf[i]);
    
    }
    printf("\n");
    // Gets all the data from the file and does the "byte stuffing"
    for(int i = 0 ; i < bufSize ; i++)
    {   
        if(buf[i] == F)
        {
            data[dataSize] = END ;
            data[dataSize+1] = SC ;
            dataSize += 2;
        }
        else if(buf[i] == END)
        {
            data[dataSize] = END ;
            data[dataSize+1] = SD ;
            dataSize += 2;
        }
        else
        {
            data[dataSize] = buf[i];
            dataSize += 1;
        }
    }
    
    // Stuffing of the end of file byte (BCC2)
    if(BCC2 == F)
    {
        data[dataSize] = END ;
        data[dataSize+1] = SC ;
        data[dataSize+2] = F ;
        dataSize += 3;
    }
    else if(BCC2 == END)
    {
        data[dataSize] = END ;
        data[dataSize+1] = SD ;
        data[dataSize+2] = F ;
        dataSize += 3;
    }
    else
    {
        data[dataSize] = BCC2 ;
        data[dataSize+1] = F ;
        dataSize += 2;
    }

    printf("\nAfter Stuffing\n");
    for(int i = 0; i < dataSize; i++) 
    { 
        printf("Sent: 0x%02x.\n",data[i]);
    
    } 
    printf("\n");
    (void) signal(SIGALRM, atende); 
    char buf2[BUF_SIZE];

    while(writeState != STOPED && alarme_count <= 3)
    {
        // Sends stuffed data
        // In case of error resends
        if(!alarme_on || writeState == REJ)
        {
            writeState = START;
            write(fd,data,dataSize);
            alarm(3);
            alarme_on = 1;
        }
        int i = 0;

        // Confirms if the date was received without errors
        if(read(fd,buf2+i,1) > 0)
        {
            writeState = updateState(writeState,buf2[i],0);
            // If an error happens we clear the read
            // Should have no more then 2 bytes in the response after BCC
            if(writeState == REJ)
            {
                read(fd,buf2+i,1);
                i++;
                read(fd,buf2+i,1);
                i++;
            }
            i++;

            if(writeState == STOPED)
            {
                printf("Datas was sucessfully sent\n");
                Ns = (Ns == 0) ? 1 : 0 ;
                break;
            }   
        }
    }

    
    alarme_on = 0;
    if(alarme_count > 3)
    {
        printf("Failled to send the data\n");
        alarme_count = 0;
        return -1;
    }   
    alarme_count = 0;

    return dataSize;
}

int llread(char *packet)
{   
    // Stuffed data
    unsigned char data[MAX_PAYLOAD_SIZE*2];
    unsigned char buf[BUF_SIZE];
    unsigned char REJ_FRAME[5];
    unsigned char RR_FRAME[5];
   
    // Size
    int readSize = 0;

    readState = START ;
    
    while (readState != STOPED)
    {   
        // Reads the received stuffed data
        // Data will be filled with the stuffed data
        if(read(fd,buf,1) > 0)
        {
            
            data[readSize] = buf[0];

            // In case the cycle does not find the "end of frame" byte
            // Some error should have occured
            if(readSize > MAX_PAYLOAD_SIZE*2)
            {
                printf("End of frame byte not found within acceptable size\n");
            
                REJ_FRAME[0] = F;
                REJ_FRAME[2] = SEND;
                if(Nr) REJ_FRAME[2] = REJ1;
                else REJ_FRAME[2] = REJ0;
                REJ_FRAME[3] = REJ_FRAME[1] ^ REJ_FRAME[2];
                REJ_FRAME[4];
                write(fd,REJ_FRAME,5);
                return -1;
            }
            
            readState = updateState(readState,data[readSize],1);
            readSize++;
            if(readState == STOPED)
            {
                printf("Data sucessfully read\n");
                break;
            }
        }
    }

    readState = START;

    int BCC2 = 0;
    if(data[readSize - 3] == SC && data[readSize - 2] == END)
    {
        BCC2 = F;
        readSize-=3; // Removes from reading both BCC bytes
    }
    if(data[readSize - 3] ==  SD && data[readSize - 2] == END)
    {
        BCC2 = END; 
        readSize-=3; // Removes from reading both BCC bytes
    }
    else 
    {
        BCC2 = data[readSize - 2];
        readSize-=2;
    }    
    
    packet[0] = data[0] ;
    packet[1] = data[1] ;
    packet[2] = data[2] ;
    packet[3] = data[3] ;

    // Destuffing
    int j = 4 ; // Staring after the header
    for(int i = j ; i < readSize ; i++ , j++)
    {
        if(data[i] == END && data[i+1] == SC)
        {
            packet[j] = F ;
            i++;
        }
        if(data[i] == END && data[i+1] == SD)
        {
            packet[j] = END ;
            i++;
        }
        else packet[j] = data[i]; 
    }

    int newBCC = 0;
    for(int i = 0 ; i < j; i++) newBCC ^= packet[i];
    printf("BCC2: 0x%02x vs newBCC: 0x%02x",BCC2,newBCC);
    

    if(newBCC == BCC2)
    {   
        RR_FRAME[0] = F;
        RR_FRAME[1] = SEND;
        RR_FRAME[4] = F;
        Nr = (Nr == 0) ? 1 : 0 ;
        if(Nr) RR_FRAME[2] = RR1 ;
        else RR_FRAME[2] = RR0 ;
        RR_FRAME[3] = RR_FRAME[1] ^ RR_FRAME[2];
    }
    else
    {   
        printf("BCC2 is wrong data could be corrupted\n");
        REJ_FRAME[0] = F;
        REJ_FRAME[1] = SEND;
        REJ_FRAME[4] = F;
        if(Nr) REJ_FRAME[2] = REJ1 ;
        else REJ_FRAME[2] = REJ0 ;
        REJ_FRAME[2] = REJ_FRAME[1] ^ REJ_FRAME[2];
        write(fd,REJ_FRAME,5);
        return -1;
    }

    
    int num = write(fd,RR_FRAME,5);
    return j ;
}

int llclose(linkLayer connectionParameters, int showStatistics)
{   
    srcParameters = connectionParameters;
    alarme_on = 0;
    alarme_count = 0;
    char buf[BUF_SIZE];

    unsigned DISC_FRAME[5];
    DISC_FRAME[0] = F;
    DISC_FRAME[1] = RECV;
    DISC_FRAME[2] = DISC;
    DISC_FRAME[3] = DISC_FRAME[2] ^ DISC_FRAME[1];
    DISC_FRAME[4] = F;

    unsigned UA_FRAME[5];
    UA_FRAME[0] = F;
    UA_FRAME[1] = SEND;
    UA_FRAME[2] = UA;
    UA_FRAME[3] = UA_FRAME[1] ^ UA_FRAME[2];
    UA_FRAME[4] = F;
    

    switch (srcParameters.role)
    {
        case 1: // Trasmitter
            (void) signal(SIGALRM,atende);
            closeState = START ;
            while(closeState != STOPED && alarme_count <= srcParameters.numTries)
            {
                if(!alarme_on)
                {
                    closeState = START;
                    write(fd,DISC_FRAME,5);
                    alarm(srcParameters.timeOut);
                    alarme_on = 1;    
                }

                if(read(fd,buf,1) > 0)
                {
                    closeState = updateState(closeState,buf[0],0);
                    if(closeState == STOPED) break;
                }
            }

            if(alarme_count == srcParameters.numTries) 
            {
                printf("Failed to correctly close connection\n");
                return -1;
            }
            write(fd,UA_FRAME,5);

        break;
    
        case 0:
            closeState = START ;
            while(closeState != STOPED)
            {
                int num = read(fd,buf,1);
                closeState = updateState(closeState,buf[0],0);
            }
            write(fd,UA_FRAME,5);
            buf[0] = '\0';
            
            closeState = START ;
            while(closeState != STOPED)
            {
                read(fd,buf,1);
                closeState = updateState(closeState,buf[0],0);
            }
        break;
        
        default:
        break;
    }

    alarme_on = 0;
    alarme_count = 0;

    sleep(1);
    
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }


    close(fd);
    return 1;

}

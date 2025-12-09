#if defined WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #define closesocket close
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <netdb.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define PROTOPORT 23452
#define BUFFERSIZE 512
#define NAMELEN 64

void ErrorHandler(char *errorMessage){
    printf("%s\n",errorMessage);
}

void ClearWinSock(){
    #if defined WIN32
        WSACleanup();
    #endif
}

void PrintClientInfo(struct sockaddr_in *addr){
    printf("Richiesta ricevuta da IP %s, Porta %d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

int main(int argc, char *argv[]){
    int port;
    if(argc>1){
        port=atoi(argv[1]);
    }else{
        port=PROTOPORT;
    }

    #if defined WIN32
        WSADATA wsaData;
        int iResult=WSAStartup(MAKEWORD(2,2), &wsaData);
        if(iResult!=0){
            ErrorHandler("Errore WSAData()");
            return -1;
        }
    #endif

    int serverSocket;
    if((serverSocket=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP))<0){
        ErrorHandler("Errore socket()");
        return -1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=htonl(INADDR_ANY);
    serverAddr.sin_port=htons(port);

    if(bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))<0){
        ErrorHandler("Errore bind()");
        closesocket(serverSocket);
        ClearWinSock();
        return -1;
    }

    printf("Server UDP in ascolto sulla porta %d\n", port);

    struct sockaddr_in clientAddr;
    socklen_t clientLen;

    char buffer[BUFFERSIZE];
    int bytesRcvd;
    char responseMsg[BUFFERSIZE];
    int num1, num2;
    double result = 0.0;
    int validOperation = 0;

    while(1){
        clientLen = sizeof(clientAddr);
        if((bytesRcvd=recvfrom(serverSocket, buffer, BUFFERSIZE-1, 0,
                               (struct sockaddr*)&clientAddr, &clientLen))<=0){
            ErrorHandler("Errore recvfrom() operazione");
            continue;
        }
        buffer[bytesRcvd]='\0';

        PrintClientInfo(&clientAddr);
        char cmd = tolower(buffer[0]);
        validOperation = 1;

        switch(cmd){
            case 'a':
                strcpy(responseMsg, "ADDIZIONE");
                break;
            case 's':
                strcpy(responseMsg, "SOTTRAZIONE");
                break;
            case 'm':
                strcpy(responseMsg, "MOLTIPLICAZIONE");
                break;
            case 'd':
                strcpy(responseMsg, "DIVISIONE");
                break;
            default:
                strcpy(responseMsg, "TERMINE PROCESSO CLIENT");
                validOperation = 0;
                break;
        }

        if(sendto(serverSocket, responseMsg, strlen(responseMsg), 0,
                  (struct sockaddr*)&clientAddr, sizeof(clientAddr)) != strlen(responseMsg)){
            ErrorHandler("Errore sendto() operazione");
            continue;
        }

        if(validOperation){

            clientLen = sizeof(clientAddr);

            if((bytesRcvd=recvfrom(serverSocket, buffer, BUFFERSIZE-1, 0,
                                   (struct sockaddr*)&clientAddr, &clientLen))<=0){
                ErrorHandler("Errore recvfrom() numeri");
                continue;
            }
            buffer[bytesRcvd]='\0';

            if(sscanf(buffer, "%d %d", &num1, &num2) == 2){
                switch(cmd){
                    case 'a':
                        result = num1 + num2;
                        break;
                    case 's':
                        result = num1 - num2;
                        break;
                    case 'm':
                        result = num1 * num2;
                        break;
                    case 'd':
                        if(num2 != 0) result = (double)num1 / num2;
                        else result = 0.0;
                        break;
                }
                sprintf(responseMsg, "%.2f", result);

                sendto(serverSocket, responseMsg, strlen(responseMsg), 0,
                       (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            } else {
                strcpy(responseMsg, "Errore nel formato numeri");
                sendto(serverSocket, responseMsg, strlen(responseMsg), 0,
                       (struct sockaddr*)&clientAddr, sizeof(clientAddr));
            }
        }
    }
}

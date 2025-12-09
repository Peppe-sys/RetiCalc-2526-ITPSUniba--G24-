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
#define QLEN 6
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

void PrintHostPort(struct sockaddr_in *addr){
    struct hostent *hostPtr;
    hostPtr=gethostbyaddr((char*) &addr->sin_addr, sizeof(addr->sin_addr), AF_INET);
    if(hostPtr==NULL){
        printf("Host sconosciuto (IP: %s)\n", inet_ntoa(addr->sin_addr));
    }else{
        printf("Host riconosciuto %s con IP %s\n", hostPtr->h_name, inet_ntoa(addr->sin_addr));
    }
}

int main(int argc, char *argv[]){
    int port;
    if(argc>1){
        port=atoi(argv[1]);
    }else{
        port=PROTOPORT;
    }
    if(port<0){
        printf("Bad port");
        return -1;
    }

    #if defined WIN32
        WSADATA wsaData;
        int iResult=WSAStartup(MAKEWORD(2,2), &wsaData);
        if(iResult!=0){
            ErrorHandler("Errore WSAData()");
            return -1;
        }
    #endif

    char server_hostname[NAMELEN];
    if(gethostname(server_hostname, sizeof(server_hostname))!=0){
        strcpy(server_hostname, "indirizzo locale sconosciuto");
    }

    int serverSocket;
    if((serverSocket=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))<0){
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

    if(listen(serverSocket, QLEN)<0 ){
        ErrorHandler("Errore listen()");
        closesocket(serverSocket);
        ClearWinSock();
        return -1;
    }

    struct sockaddr_in clientAddr;
    int clientSocket;
    socklen_t clientLen = sizeof(clientAddr);
    char buffer[BUFFERSIZE];
    int bytesRcvd;
    char responseMsg[BUFFERSIZE];
    int num1, num2;
    double result = 0.0;
    int validOperation = 0;

    while(1){
        if((clientSocket=accept(serverSocket, (struct sockaddr*) &clientAddr, &clientLen))<0){
            ErrorHandler("Errore accept()");
            continue;
        }

        PrintHostPort(&clientAddr);

        strcpy(responseMsg, "connessione avvenuta");
        if(send(clientSocket, responseMsg, strlen(responseMsg), 0) != strlen(responseMsg)){
            ErrorHandler("Errore send() handshake");
            closesocket(clientSocket);
            continue;
        }

        if((bytesRcvd=recv(clientSocket, buffer, BUFFERSIZE-1, 0))<=0){
            ErrorHandler("Errore recv() lettera");
            closesocket(clientSocket);
            continue;
        }
        buffer[bytesRcvd]='\0';

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

        if(send(clientSocket, responseMsg, strlen(responseMsg), 0) != strlen(responseMsg)){
            ErrorHandler("Errore send() operazione");
            closesocket(clientSocket);
            continue;
        }

        //Punto 9
        if(validOperation){
            if((bytesRcvd=recv(clientSocket, buffer, BUFFERSIZE-1, 0))<=0){
                ErrorHandler("Errore recv() numeri");
                closesocket(clientSocket);
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
                send(clientSocket, responseMsg, strlen(responseMsg), 0);
            } else {
                strcpy(responseMsg, "Errore nel formato numeri");
                send(clientSocket, responseMsg, strlen(responseMsg), 0);
            }
        }

        closesocket(clientSocket);
    }
}

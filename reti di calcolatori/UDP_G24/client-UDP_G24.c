#if defined(_WIN32)
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

void ErrorHandler(char *errorMessage){
    printf("%s\n",errorMessage);
}

void ClearWinSock(){
    #if defined(_WIN32)
        WSACleanup();
    #endif
}

int main (int argc, char *argv[]){
    int port;
    char hostName[64];

    if(argc == 3){
        strcpy(hostName, argv[1]);
        port = atoi(argv[2]);
    } else {
        printf("Inserisci nome server (es. localhost): ");
        scanf("%s", hostName);
        port = PROTOPORT;
        while(getchar() != '\n');
    }

    #if defined(_WIN32)
    WSADATA wsaData;
    int iResult=WSAStartup(MAKEWORD(2,2), &wsaData);
    if(iResult != 0){
        ErrorHandler("Errore WSAStartup()");
        return -1;
    }
    #endif

    struct hostent *hostPtr;
    hostPtr = gethostbyname(hostName);
    if (hostPtr == NULL) {
        ErrorHandler("Errore gethostbyname() - Nome host non risolvibile");
        ClearWinSock();
        return -1;
    }

    int clientSocket;
    clientSocket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(clientSocket < 0){
        ErrorHandler("Errore socket()");
        return -1;
    }

    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family=AF_INET;
    serverAddr.sin_port=htons(port);
    memcpy(&serverAddr.sin_addr, hostPtr->h_addr, hostPtr->h_length);

    char buffer[BUFFERSIZE];
    int bytesRcvd;
    struct sockaddr_in fromAddr;
    socklen_t fromSize = sizeof(fromAddr);

    printf("\n--- MENU OPERAZIONI ---\n");
    printf("Inserisci operazione (Inserisci lettera diversa per uscire):\n A = Addizione\n S = Sottrazione\n M = Moltiplicazione\n D = Divisione\n");
    printf("La tua scelta: ");

    char operationMsg[BUFFERSIZE];
    scanf("%c", operationMsg);
    operationMsg[1] = '\0';

    if(sendto(clientSocket, operationMsg, strlen(operationMsg), 0,
              (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != strlen(operationMsg)){
        ErrorHandler("errore sendto()");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }

    if((bytesRcvd=recvfrom(clientSocket, buffer, BUFFERSIZE-1, 0,
                           (struct sockaddr*)&fromAddr, &fromSize))<=0){
        ErrorHandler("Errore recvfrom()");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }
    buffer[bytesRcvd]='\0';
    printf("Risposta dal Server: %s\n", buffer);

    int isOperation = 0;
    if(strstr(buffer, "ADDIZIONE") || strstr(buffer, "SOTTRAZIONE") ||
       strstr(buffer, "MOLTIPLICAZIONE") || strstr(buffer, "DIVISIONE")) {
        isOperation = 1;
    }

    if(isOperation){
        int n1, n2;

        printf("\n--- INSERIMENTO DATI ---\n");
        printf("Inserisci il PRIMO numero intero: ");
        scanf("%d", &n1);

        printf("Inserisci il SECONDO numero intero: ");
        scanf("%d", &n2);

        char numStr[BUFFERSIZE];
        sprintf(numStr, "%d %d", n1, n2);
        if(sendto(clientSocket, numStr, strlen(numStr), 0,
                  (struct sockaddr*)&serverAddr, sizeof(serverAddr)) != strlen(numStr)){
            ErrorHandler("Errore sendto() numeri");
        }

        if((bytesRcvd=recvfrom(clientSocket, buffer, BUFFERSIZE-1, 0,
                               (struct sockaddr*)&fromAddr, &fromSize)) > 0){
            buffer[bytesRcvd]='\0';
            printf("\nRISULTATO: %s\n", buffer);
        }
    } else {
        printf("Operazione non valida.\n");
    }

    printf("Processo client terminato.\n");
    closesocket(clientSocket);
    ClearWinSock();
    #if defined(_WIN32)
       system ("pause");
    #endif
    return 0;
}

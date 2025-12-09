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
    struct hostent *hostPtr;
    int clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[BUFFERSIZE];
    int bytesRcvd;
    char operationMsg[BUFFERSIZE];
    int isOperation = 0;
    int n1, n2;
    char numStr[BUFFERSIZE];

    #if defined(_WIN32)
    WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(iResult != 0){
        ErrorHandler("Errore WSAStartup()");
        return -1;
    }
    #endif

    if(argc == 3){
        strcpy(hostName, argv[1]);
        port = atoi(argv[2]);
    } else {
        printf("Inserisci nome server (es. localhost): ");
        scanf("%s", hostName);
        port = PROTOPORT;
        while(getchar() != '\n');
    }

    hostPtr = gethostbyname(hostName);
    if (hostPtr == NULL) {
        ErrorHandler("Errore gethostbyname() - Nome host non risolvibile");
        ClearWinSock();
        return -1;
    }

    clientSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(clientSocket < 0){
        ErrorHandler("Errore socket()");
        return -1;
    }

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    memcpy(&serverAddr.sin_addr, hostPtr->h_addr, hostPtr->h_length);

    if(connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr))){
        ErrorHandler("Errore connect()");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }

    if((bytesRcvd=recv(clientSocket, buffer, BUFFERSIZE-1, 0))<=0){
        ErrorHandler("Errore recv() handshake");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }
    buffer[bytesRcvd] = '\0';
    printf("Server: %s\n", buffer);

    printf("\n--- MENU OPERAZIONI ---\n");
	printf("Inserisci operazione (Inserisci lettera diversa per uscire):\n A = Addizione\n S = Sottrazione\n M = Moltiplicazione\n D = Divisione\n");
	printf("La tua scelta: ");

    scanf(" %c", operationMsg);
    operationMsg[1] = '\0';

    if(send(clientSocket, operationMsg, strlen(operationMsg), 0) != strlen(operationMsg)){
        ErrorHandler("errore send() operazione");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }

    if((bytesRcvd=recv(clientSocket, buffer, BUFFERSIZE-1, 0))<=0){
        ErrorHandler("Errore recv() risposta operazione");
        closesocket(clientSocket);
        ClearWinSock();
        return -1;
    }
    buffer[bytesRcvd] = '\0';
    printf("Risposta dal Server: %s\n", buffer);

    if(strstr(buffer, "ADDIZIONE") || strstr(buffer, "SOTTRAZIONE") ||
       strstr(buffer, "MOLTIPLICAZIONE") || strstr(buffer, "DIVISIONE")) {
        isOperation = 1;
    }

    if(isOperation){
        printf("\n--- INSERIMENTO DATI ---\n");
        printf("Inserisci il PRIMO numero intero: ");
        scanf("%d", &n1);

        printf("Inserisci il SECONDO numero intero: ");
        scanf("%d", &n2);

        sprintf(numStr, "%d %d", n1, n2);

        if(send(clientSocket, numStr, strlen(numStr), 0) != strlen(numStr)){
            ErrorHandler("Errore send() numeri");
        }

        if((bytesRcvd=recv(clientSocket, buffer, BUFFERSIZE-1, 0)) > 0){
            buffer[bytesRcvd] = '\0';
            printf("\n>>> RISULTATO: %s\n", buffer);
        }
    } else {
        printf("Nessuna operazione valida selezionata o connessione chiusa.\n");
    }

    closesocket(clientSocket);
    ClearWinSock();
    printf("\nProcesso client terminato.\n");

    #if defined(_WIN32)
        system("pause");
    #endif
    return 0;
}

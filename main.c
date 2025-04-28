#include <stdio.h>
#include <windows.h>
#include <winsock.h>
#include <string.h>

#pragma comment(lib, "ws2_32")

int main(){
    WSADATA d;
    int start = WSAStartup(0x202, &d);
    if(start != 0){
        printf("Blad!\n");
        return 1;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s == INVALID_SOCKET){
        printf("Nie dziala socket\n");
        WSACleanup();
        return 2;
    }

    char ip[] = {142,250,190,46}; 
    struct sockaddr_in serwer;
    serwer.sin_family = AF_INET;
    serwer.sin_port = htons(80);
    serwer.sin_addr.S_un.S_addr = *(unsigned long*)ip;

    int polacz = connect(s, (struct sockaddr*)&serwer, sizeof(serwer));
    if(polacz == SOCKET_ERROR){
        printf("Nie polaczono :(\n");
        closesocket(s);
        WSACleanup();
        return 3;
    } else {
        printf("Poloczono\n");
    }

  
    char request[] = "GET / HTTP/1.1\r\nHost: google.com\r\nConnection: close\r\n\r\n";
    int wyslano = send(s, request, strlen(request), 0);
    if(wyslano == SOCKET_ERROR){
        printf("Blad wysylania\n");
        closesocket(s);
        WSACleanup();
        return 4;
    }

    char buffer[1024];
    int odebrano;
    int totalBytes = 0;  
    printf("Odpowiedz serwera:\n");
    do {
        odebrano = recv(s, buffer, sizeof(buffer) - 1, 0);
        if(odebrano > 0){
            totalBytes += odebrano; 
            buffer[odebrano] = '\0';
            printf("%s", buffer);
        }
    } while(odebrano > 0);

    printf("\n\nLacznie odebrano %d bajtow danych.\n", totalBytes);

    closesocket(s);
    WSACleanup();

    system("pause");
    return 0;
}

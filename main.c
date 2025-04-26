#include <stdio.h>
#include <windows.h>
#include <winsock.h>

#pragma comment(lib, "ws2_32")

int main(){
    WSADATA d;
    int start = WSAStartup(0x202, &d);
    if(start != 0){
        printf("Blad!\n");
        return 1;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(s == -1){
        printf("Nie dziala socket\n");
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
    } else {
        printf("poloczono\n");
    }

    closesocket(s);
    WSACleanup();

    system("pause");
    return 0;
}
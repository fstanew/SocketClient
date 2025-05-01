#include <stdio.h>
#include <windows.h>
#include <winsock.h>
#include <string.h>

#pragma comment(lib, "ws2_32")

int main() {
    WSADATA d;
    int start = WSAStartup(0x202, &d);
    if (start != 0) {
        printf("Blad!\n");
        return 1;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        printf("Nie dziala socket\n");
        WSACleanup();
        return 2;
    }

    char host[100];
    char sciezka[200];
    char slowo[50];

    printf("Podaj hosta (np. google.com): ");
    scanf("%99s", host);

    printf("Podaj sciezke (np. / lub /search): ");
    scanf("%199s", sciezka);

    printf("Podaj slowo do zliczenia w odpowiedzi (np. html): ");
    scanf("%49s", slowo);

    struct hostent *h = gethostbyname(host);
    if (h == NULL) {
        printf("Nie znaleziono hosta\n");
        closesocket(s);
        WSACleanup();
        return 3;
    }

    struct sockaddr_in serwer;
    serwer.sin_family = AF_INET;
    serwer.sin_port = htons(80);
    serwer.sin_addr = *(struct in_addr*)h->h_addr;

    int polacz = connect(s, (struct sockaddr*)&serwer, sizeof(serwer));
    if (polacz == SOCKET_ERROR) {
        printf("Nie polaczono :(\n");
        closesocket(s);
        WSACleanup();
        return 4;
    }
    else {
        printf("Poloczono z %s\n", host);
    }

    // Start pomiaru czasu
    DWORD startTime = GetTickCount();

    char request[512];
    snprintf(request, sizeof(request),
        "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
        sciezka, host);

    int wyslano = send(s, request, strlen(request), 0);
    if (wyslano == SOCKET_ERROR) {
        printf("Blad wysylania\n");
        closesocket(s);
        WSACleanup();
        return 5;
    }

    FILE* plik = fopen("response.txt", "w");
    if (plik == NULL) {
        printf("Nie mozna otworzyc pliku do zapisu!\n");
        closesocket(s);
        WSACleanup();
        return 6;
    }

    char buffer[1024];
    int odebrano;
    int totalBytes = 0;
    int licznikWystapien = 0;

    printf("Odpowiedz serwera:\n");
    do {
        odebrano = recv(s, buffer, sizeof(buffer) - 1, 0);
        if (odebrano > 0) {
            totalBytes += odebrano;
            buffer[odebrano] = '\0';
            printf("%s", buffer);
            fprintf(plik, "%s", buffer);

            // Liczenie wystąpień słowa
            char* ptr = buffer;
            while ((ptr = strstr(ptr, slowo)) != NULL) {
                licznikWystapien++;
                ptr += strlen(slowo);
            }
        }
    } while (odebrano > 0);

    fclose(plik);

    DWORD endTime = GetTickCount();
    printf("\n\nLacznie odebrano %d bajtow danych.\n", totalBytes);
    printf("Czas trwania odpowiedzi: %lu ms\n", endTime - startTime);
    printf("Liczba wystapien slowa \"%s\": %d\n", slowo, licznikWystapien);

    closesocket(s);
    WSACleanup();

    system("pause");
    return 0;
}

#include <windows.h>
#include <winsock.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32")

#define PROGRESS_BAR_WIDTH 50

static char *strcasestr_local(const char *h, const char *n) {
    if (!*n) return (char *)h;
    size_t nlen = strlen(n);
    for (const char *p = h; *p; ++p)
        if (_strnicmp(p, n, nlen) == 0)
            return (char *)p;
    return NULL;
}

static void update_progress_bar(int downloaded, int total) {
    if (total <= 0) return;
    float fraction = (float)downloaded / total;
    int filled = (int)(fraction * PROGRESS_BAR_WIDTH);
    printf("\r[");
    for (int i = 0; i < PROGRESS_BAR_WIDTH; ++i)
        putchar(i < filled ? '#' : ' ');
    printf("] %3d%%", (int)(fraction * 100));
    fflush(stdout);
}

int main(void) {
    WSADATA d;
    if (WSAStartup(0x202, &d) != 0) return 1;

    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) {
        WSACleanup();
        return 2;
    }

    char host[100], sciezka[200], slowo[50];
    printf("Podaj hosta (np. google.com): ");
    scanf("%99s", host);
    printf("Podaj sciezke (np. / lub /search): ");
    scanf("%199s", sciezka);
    printf("Podaj slowo do zliczenia w odpowiedzi (np. html): ");
    scanf("%49s", slowo);

    struct hostent *h = gethostbyname(host);
    if (!h) {
        closesocket(s);
        WSACleanup();
        return 3;
    }

    struct sockaddr_in serwer = {0};
    serwer.sin_family = AF_INET;
    serwer.sin_port = htons(80);
    serwer.sin_addr = *(struct in_addr *)h->h_addr;

    if (connect(s, (struct sockaddr *)&serwer, sizeof(serwer)) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return 4;
    }

    DWORD startTime = GetTickCount();

    char request[512];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             sciezka, host);

    if (send(s, request, strlen(request), 0) == SOCKET_ERROR) {
        closesocket(s);
        WSACleanup();
        return 5;
    }

    FILE *plik = fopen("response.txt", "w");
    if (!plik) {
        closesocket(s);
        WSACleanup();
        return 6;
    }

    FILE *matches = fopen("matches.txt", "w");
    if (!matches) {
        fclose(plik);
        closesocket(s);
        WSACleanup();
        return 7;
    }

    char buffer[2048];
    int odebrano, totalBytes = 0, licznikWystap = 0, headerDone = 0;
    int contentLength = -1, bodyBytes = 0;
    char headerBuf[8192] = {0};
    int headerLen = 0;

    char longestLine[2048] = {0};
    int maxLineLength = 0;

    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    printf("Odpowiedz serwera:\n");

    while ((odebrano = recv(s, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[odebrano] = '\0';
        totalBytes += odebrano;

        if (!headerDone) {
            if (headerLen + odebrano < (int)sizeof(headerBuf) - 1) {
                memcpy(headerBuf + headerLen, buffer, odebrano);
                headerLen += odebrano;
                headerBuf[headerLen] = '\0';
            }

            char *headerEnd = strstr(headerBuf, "\r\n\r\n");
            if (!headerEnd) continue;

            headerDone = 1;

            char *cl = strcasestr_local(headerBuf, "Content-Length:");
            if (cl) contentLength = atoi(cl + 15);

            fwrite(headerBuf, 1, (headerEnd + 4) - headerBuf, plik);

            int headerBytesThisChunk = (headerEnd + 4) - (headerBuf + headerLen - odebrano);
            int bodyOffset = odebrano - headerBytesThisChunk;
            if (bodyOffset > 0) {
                char *ptr = buffer + headerBytesThisChunk;
                bodyBytes += bodyOffset;
                fwrite(ptr, 1, bodyOffset, plik);

                while (*ptr && ptr < buffer + odebrano) {
                    char *found = strstr(ptr, slowo);
                    if (found && found < buffer + odebrano) {
                        fwrite(ptr, 1, found - ptr, stdout);
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                        fwrite(found, 1, strlen(slowo), stdout);
                        SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                        licznikWystap++;

                        char *line_start = found;
                        while (line_start > buffer && *(line_start - 1) != '\n' && *(line_start - 1) != '\r') line_start--;
                        char *line_end = found;
                        while (line_end < buffer + odebrano && *line_end != '\n') line_end++;
                        fprintf(matches, "%.*s\n", (int)(line_end - line_start), line_start);

                        if ((int)(line_end - line_start) > maxLineLength) {
                            maxLineLength = (int)(line_end - line_start);
                            snprintf(longestLine, sizeof(longestLine), "%.*s", maxLineLength, line_start);
                        }

                        ptr = found + strlen(slowo);
                    } else {
                        fwrite(ptr, 1, (buffer + odebrano) - ptr, stdout);
                        break;
                    }
                }
            }
        } else {
            bodyBytes += odebrano;
            fwrite(buffer, 1, odebrano, plik);

            char *ptr = buffer;
            while (*ptr) {
                char *found = strstr(ptr, slowo);
                if (found) {
                    fwrite(ptr, 1, found - ptr, stdout);
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_INTENSITY);
                    fwrite(found, 1, strlen(slowo), stdout);
                    SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
                    licznikWystap++;

                    char *line_start = found;
                    while (line_start > buffer && *(line_start - 1) != '\n' && *(line_start - 1) != '\r') line_start--;
                    char *line_end = found;
                    while (line_end < buffer + odebrano && *line_end != '\n') line_end++;
                    fprintf(matches, "%.*s\n", (int)(line_end - line_start), line_start);

                    if ((int)(line_end - line_start) > maxLineLength) {
                        maxLineLength = (int)(line_end - line_start);
                        snprintf(longestLine, sizeof(longestLine), "%.*s", maxLineLength, line_start);
                    }

                    ptr = found + strlen(slowo);
                } else {
                    printf("%s", ptr);
                    break;
                }
            }
        }

        if (contentLength > 0)
            update_progress_bar(bodyBytes, contentLength);
    }

    if (contentLength > 0) putchar('\n');

    fclose(plik);
    fclose(matches);

    FILE *longest = fopen("longest_match.txt", "w");
    if (longest) {
        fprintf(longest, "%s\n", longestLine);
        fclose(longest);
        printf("\nNajdluzsza linia zawierajaca \"%s\":\n%s\n", slowo, longestLine);
    } else {
        printf("Nie udalo sie zapisac longest_match.txt\n");
    }

    DWORD endTime = GetTickCount();
    DWORD elapsed = endTime - startTime;
    printf("\n\nLacznie odebrano %d bajtow danych.\n", totalBytes);
    printf("Czas trwania odpowiedzi: %lu ms\n", elapsed);
    printf("Liczba wystapien slowa \"%s\": %d\n", slowo, licznikWystap);

    // --- Dodana funkcjonalność ---
    FILE *summary = fopen("summary.txt", "w");
    if (summary) {
        fprintf(summary, "Host: %s\nSciezka: %s\nSzukane slowo: %s\n", host, sciezka, slowo);
        fprintf(summary, "Liczba wystapien: %d\n", licznikWystap);
        fprintf(summary, "Czas odpowiedzi: %lu ms\n", elapsed);
        fclose(summary);
        printf("Podsumowanie zapisane w summary.txt\n");
    } else {
        printf("Nie udalo sie zapisac summary.txt\n");
    }

    closesocket(s);
    WSACleanup();
    system("pause");
    return 0;
}

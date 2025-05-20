#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 1024 // 버퍼 크기 

void ErrorHandling(const char* msg); // 오류 발생 시 메시지를 출력하고 프로그램을 종료하는 함수
void Login(SOCKET sock);             // 서버에 아이디/패스워드를 전송하고 로그인하는 함수
int handleResponse(SOCKET sock);     // 서버 응답 처리 함수

int main() {
    SetConsoleCP(65001);       // 콘솔 입력 인코딩을 UTF-8로 설정
    SetConsoleOutputCP(65001); // 콘솔 출력 인코딩을 UTF-8로 설정

    WSADATA wsaData;
    SOCKET sock;
    SOCKADDR_IN serverAddr;
    
    char serverIp[] = "127.0.0.1"; // 서버 IP 주소 
    char port[] = "55555";         // 서버 포트 번호

    // 윈도우 소켓 라이브러리리 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
        ErrorHandling("WSAStartup failed");

    // 소켓 생성
    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
        ErrorHandling("socket() error");

    // 서버 주소 설정
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);
    serverAddr.sin_port = htons(atoi(port));

    // 서버 연결 시도
    if (connect(sock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
        ErrorHandling("connect() error");

    Login(sock); // 로그인 처리

    // 서버와 통신 루프
    while (1) {
        if (!handleResponse(sock)) break; // 서버 응답 처리, 종료 시 루프 탈출
    }

    closesocket(sock); // 소켓 종료
    WSACleanup(); // 윈도우 소켓 라이브러리 종료

    return 0;
}

void Login(SOCKET sock) {
    char msg[BUF_SIZE], recvBuf[BUF_SIZE];

    int ret;

    while (1) {
         // 서버로부터 "ID : "  수신
        ret = recv(sock, recvBuf, BUF_SIZE - 1, 0);

        if (ret <= 0) {
            ErrorHandling("recv() error");
        }

        recvBuf[ret] = '\0';

        printf("%s", recvBuf);
        
        // 입력 후 서버로 전송
        fgets(msg, BUF_SIZE, stdin);
        msg[strcspn(msg, "\r\n")] = '\0';
        send(sock, msg, strlen(msg), 0);

        // 서버로부터 "Password : " 수신
        ret = recv(sock, recvBuf, BUF_SIZE - 1, 0);

        if (ret <= 0) {
            ErrorHandling("recv() error");
        }

        recvBuf[ret] = '\0';

        printf("%s", recvBuf);

        // 입력 후 서버로 전송
        fgets(msg, BUF_SIZE, stdin);
        msg[strcspn(msg, "\r\n")] = '\0';
        send(sock, msg, strlen(msg), 0);

        // 서버로부터 결과 수신
        ret = recv(sock, recvBuf, BUF_SIZE - 1, 0);

        if (ret <= 0) {
            ErrorHandling("recv() error");
        }

        recvBuf[ret] = '\0';

        printf("%s\n", recvBuf);

        // 로그인 성공 시
        if (strstr(recvBuf, "로그인 성공") != NULL) {
            return;
        }
        // 로그인 실패 시
        printf("로그인 실패, 다시 시도하세요.\n");
    }
}

int handleResponse(SOCKET sock) {
    char msg[BUF_SIZE], buf[BUF_SIZE];

    int ret;

    while (1) {
        // 반복적으로 서버 출력 받기
        while (1) {
            ret = recv(sock, buf, BUF_SIZE - 1, 0);
            // 오류 시 함수 종료
            if (ret <= 0) {
                return 0;
            }
      
            buf[ret] = '\0';
            // 서버 메시지 출력
            printf("%s", buf);
            // [입력] 또는 [기능 종료]가 포함되면 루프 탈출
            if (strstr(buf, "[입력]") != NULL || strstr(buf, "[기능 종료]") != NULL) {
                break;
            }
        }

        fgets(msg, BUF_SIZE, stdin); // 입력 받기
        msg[strcspn(msg, "\r\n")] = '\0'; // 개행 문자 제거

        if (strlen(msg) == 0) {
            strcpy(msg, "<ENTER>"); // 엔터만 입력한 경우 <ENTER>로 대체
        }
        // 0 입력 시 메뉴로 돌아감
        if (strcmp(msg, "0") == 0) {
            send(sock, msg, strlen(msg), 0);
            printf("메뉴로 돌아갑니다.\n");
            return 1;
        }
        // 서버에 입력값 전송
        if (send(sock, msg, strlen(msg), 0) == SOCKET_ERROR)
            ErrorHandling("send() error");

        return 1;
    }

    return 1;
}
// 오류 발생 시 메시지를 출력하고 프로그램 종료
void ErrorHandling(const char* msg) {
    fprintf(stderr, "[ERROR] %s (code: %d)\n", msg, WSAGetLastError());
    exit(1);
}
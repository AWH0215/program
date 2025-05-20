#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include <windows.h> 
#include <process.h>
#include <ctype.h>

#pragma warning(disable:6031)

#define MAX_BOOKS 700                           // 최대 도서 수
#define FILE_PATH "C:\\Temp\\booklist2.txt"     // 도서 정보 파일
#define TEMP_PATH "C:\\Temp\\temp_booklist.txt" // 임시 도서 정보 파일

#define MAX_USERS 10                            // 최대 사용자 수
#define USER_FILE_PATH "C:\\Temp\\users.txt"    // 사용자 장버 파일

#define BUF_SIZE 1024                           // 버퍼 크기
#define MAX_CLNT 10	                            // 동시에 접속 가능한 클라이언트 수

// 도서 정보를 저장하는 구조체
typedef struct BOOK {
	int num;             // 도서의 순번
	char booktitle[150]; // 도서의 제목
	char author[150];    // 도서의 저자
	float rating;        // 도서의 평점
} BOOK;
// 사용자 정보를 저장하는 구조체
typedef struct USER {
	char id[15]; // 사용자 아이디
	char pw[15]; // 사용자 패스워드
} USER;

BOOK books[MAX_BOOKS]; // 전체 도서 배열

int book = 0; // 현재 저장된 도서 수(추가 시 증가, 삭제 시 감소)

void loadbooks(BOOK* books, int* book);        // 도서 정보 파일 읽기    
void booklist(BOOK* books, int* book);         // 도서 목록 출력
void searchbook(BOOK* books, int* book);       // 도서 검색
void addbook(BOOK* books, int* book);          // 도서 추가
void deletebook(BOOK* books, int* book);       // 도서 삭제
void modifybook(BOOK* books, int* book);       // 도서 수정
int ratingbooks(const void* a, const void* b); // 평점 내림차 순 정렬
void rankbooks(BOOK* books, int* book);        // 평점 순 랭킹

USER users[MAX_USERS]; // 전체 사용자 배열

int user = 0; // 현재 저장된 사용자 수(추가 시 증가, 삭제 시 감소)

void loadusers(USER* users, int* user);  // 사용자 정보 파일 읽기
void userlist(USER* users, int* user);   // 사용자 정보 목록 출력
void adduser(USER* users, int* user);    // 사용자 정보 추가
void deleteuser(USER* users, int* user); // 사용자 정보 삭제
void modifyuser(USER* users, int* user); // 사용자 정보 수정

int clientcount = 0;          // 현재 접속 중인 클라이언트 수

SOCKET clientsocks[MAX_CLNT]; // 클라이언트 소켓 저장 배열

SOCKET serversocket;          // 서버 소켓

HANDLE hmutex;                // 뮤텍스

unsigned WINAPI HandleClient(void* arg); // 클라이언트 요청을 처리하는 함수
unsigned WINAPI SERVER(void* arg);       // 클라이언트 연결 수락 함수
void SendMsg(char* msg, int len);        // 메시지 전송 함수
void ErrorHandling(char* msg);           // 오류 발생 시 메시지를 출력하고 프로그램을 종료하는 함수

// 클라이언트에게 메뉴 전송하는 함수수
void SendMenu(SOCKET clntSock) {
	Sleep(50);
    send(clntSock,
        "\r\n-------- 도서 정보 관리 프로그램 --------\r\n"
        "1. 도서 정보 목록 보기\r\n"
        "2. 도서 정보 검색\r\n"
        "3. 도서 정보 추가\r\n"
        "4. 도서 정보 삭제\r\n"
        "5. 도서 정보 수정\r\n"
        "6. 평점 순 랭킹\r\n"
        "7. 로그아웃\r\n"
		"-----------------------------------------\r\n"
        "[입력] 사용할 기능의 번호 : ",
        strlen(
            "\r\n-------- 도서 정보 관리 프로그램 --------\r\n"
            "1. 도서 정보 목록 보기\r\n"
            "2. 도서 정보 검색\r\n"
            "3. 도서 정보 추가\r\n"
            "4. 도서 정보 삭제\r\n"
            "5. 도서 정보 수정\r\n"
            "6. 평점 순 랭킹\r\n"
            "7. 로그아웃\r\n"
			"-----------------------------------------\r\n"
            "[입력] 사용할 기능의 번호 : "),
        0);
}

// 클라이언트 요청을 처리하는 함수
unsigned WINAPI HandleClient(void* arg) {
	SOCKET clntSock = *((SOCKET*)arg);                        // 클라이언트 소켓 받기

	int strLen;

	char msg[BUF_SIZE];

	// 클라이언트에 "ID : "  전송
	send(clntSock, "ID : ", strlen("ID : "), 0);
    // 클라이언트로부터 ID 수신
	strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
	if (strLen <= 0) { 
		closesocket(clntSock); 
		return 0; 
	}
	msg[strLen] = '\0';
	msg[strcspn(msg, "\r\n")] = '\0';
    // ID 복사
	char id[15]; 
	strcpy(id, msg);
    // 클라이언트에 "Password : "  전송
	send(clntSock, "Password : ", strlen("Password : "), 0);
    // 클라이언트로부터 Password 수신
	strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
	if (strLen <= 0) { 
		closesocket(clntSock); 
		return 0; 
	}
	msg[strLen] = '\0';
	msg[strcspn(msg, "\r\n")] = '\0'; 
    // Password 복사
	char pw[15]; 
	strcpy(pw, msg);

	// 사용자 인증
	loadusers(users, &user); // 사용자 정보 읽기

	int check = 0; // 로그인 성공 확인 (1이면 성공, 0이면 실패)

	for (int i = 0; i < user; i++) {
		if (strcmp(users[i].id, id) == 0 &&
			strcmp(users[i].pw, pw) == 0) {
			check = 1; // 일치 시 1

			break;
		}
	}
	// 로그인 실패 시
	if (!check) {
		send(clntSock, "로그인 실패\r\n", strlen("로그인 실패\r\n"), 0);

		closesocket(clntSock);

		return 0;
	}
	// 로그인 성공 시
	send(clntSock, "로그인 성공\r\n", strlen("로그인 성공\r\n"), 0);
	Sleep(20);
	SendMenu(clntSock);

	// 메뉴 (1~7)
	while (1) {
		// 클라이언트로부터 번호 수신
		strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
        // 연결 종료 또는 오류 시
		if (strLen <= 0) {
			break;
		}
		msg[strLen] = '\0';
		msg[strcspn(msg, "\r\n")] = '\0';

		// 7. 로그아웃
		if (strcmp(msg, "7") == 0) {
			send(clntSock, "로그아웃 합니다.\r\n", strlen("로그아웃 합니다.\r\n"), 0);

			break;
		}

		// 1. 도서 정보 목록 보기
		if (strcmp(msg, "1") == 0) {
			loadbooks(books, &book);
		
			if (book == 0) {
				send(clntSock, "출력할 도서가 없습니다.\r\n", strlen("출력할 도서가 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
		
			// 범위 설정 가능 (현재 도서 수 제공)
			char rangeMsg[128];
			sprintf(rangeMsg, "[입력] 도서 출력 범위 지정 (1 ~ %d) : ", book);
			send(clntSock, rangeMsg, strlen(rangeMsg), 0);
		
			// 클라이언트로부터 범위 수신
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);

			if (strLen <= 0) {
				break;
			}

			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
		
			int min, max;
            // 입력 범위 확인
			if (sscanf(msg, "%d %d", &min, &max) != 2 || min < 1 || max > book || min > max) {
				send(clntSock, "잘못된 범위입니다. 유효한 숫자 2개를 입력하세요.\r\n", 
					strlen("잘못된 범위입니다. 유효한 숫자 2개를 입력하세요.\r\n"), 0); 
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			// 범위 내 도서 정보 출력
			for (int i = min - 1; i < max; i++) {
				char line[512];
				sprintf(line, "%-5d\t%-50s\t%-50s\t%-5.2f\r\n",
					books[i].num,
					books[i].booktitle,
					books[i].author,
					books[i].rating);
				send(clntSock, line, strlen(line), 0);
			}
		
			Sleep(20);
			SendMenu(clntSock);
			continue;
		}
		// 2. 도서 정보 검색
		else if (strcmp(msg, "2") == 0) {
			// 검색 방법
			send(clntSock,
				"[입력] 검색 방법을 선택하세요\r\n"
				"1. 순번으로 검색\r\n"
				"2. 제목으로 검색\r\n"
				"3. 저자로 검색\r\n"
				"선택 : ",
				strlen(
					"[입력] 검색 방법을 선택하세요\r\n"
					"1. 순번으로 검색\r\n"
					"2. 제목으로 검색\r\n"
					"3. 저자로 검색\r\n"
					"선택 : "),
				0);
            // 검색 방법 수신
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';

			loadbooks(books, &book); // 도서 정보 파일 읽기

			int check = 0;           // 검색 성공 확인 (1이면 성공, 0이면 실패)
            // 순번으로 검색
			if (strcmp(msg, "1") == 0) { 
				send(clntSock, "[입력] 도서 순번 (0 입력 시 돌아가기) : ", strlen("[입력] 도서 순번 (0 입력 시 돌아가기) : "), 0);
				strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
				msg[strLen] = '\0';
				msg[strcspn(msg, "\r\n")] = '\0';
				int booknum = atoi(msg);
                // "0" 입력 시 메뉴로 돌아감
				if (booknum == 0) {
					Sleep(20);
					SendMenu(clntSock);
					continue;
				}
                // 같은 순번 도서 찾기
				for (int i = 0; i < book; i++) {
					if (books[i].num == booknum) {
						char line[512];
						sprintf(line, "%-5d\t%-50s\t%-50s\t%-5.2f\r\n",
							books[i].num,
							books[i].booktitle,
							books[i].author,
							books[i].rating);
						send(clntSock, line, strlen(line), 0);
						check = 1;
						break;
					}
				}
                // 도서 찾기 실패
				if (!check) {
					send(clntSock, "해당 순번의 도서를 찾을 수 없습니다.\r\n", strlen("해당 순번의 도서를 찾을 수 없습니다.\r\n"), 0);
				}
			}
			// 제목으로 검색
			else if (strcmp(msg, "2") == 0) { 
				send(clntSock, "[입력] 도서 제목 (돌아가기 입력 시 돌아가기): ", strlen("[입력] 도서 제목 (돌아가기 입력 시 돌아가기): "), 0);
				strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
				msg[strLen] = '\0';
				msg[strcspn(msg, "\r\n")] = '\0';
				// "돌아가기" 입력 시 메뉴로 돌아감
				if (strcmp(msg, "돌아가기") == 0) {
					Sleep(20);
					SendMenu(clntSock);
					continue;
				}
				char booktitle[50];
				strcpy(booktitle, msg);
                // 같은 제목 도서 찾기
				for (int i = 0; i < book; i++) {
					if (_stricmp(booktitle, books[i].booktitle) == 0) {
						char line[512];
						sprintf(line, "%-5d\t%-50s\t%-50s\t%-5.2f\r\n",
							books[i].num,
							books[i].booktitle,
							books[i].author,
							books[i].rating);
						send(clntSock, line, strlen(line), 0);
						check = 1;
						break;
					}
				}
                // 도서 찾기 실패
				if (!check) {
					send(clntSock, "해당 제목을 가지는 도서를 찾을 수 없습니다.\r\n", strlen("해당 제목을 가지는 도서를 찾을 수 없습니다.\r\n"), 0);
				}
			}
			// 저자로 검색
			else if (strcmp(msg, "3") == 0) { 
				send(clntSock, "[입력] 저자명 (돌아가기 입력 시 돌아가기): ", strlen("[입력] 저자명 (돌아가기 입력 시 돌아가기): "), 0);
				strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
				msg[strLen] = '\0';
				msg[strcspn(msg, "\r\n")] = '\0';
				// "돌아가기" 입력 시 메뉴로 돌아감
				if (strcmp(msg, "돌아가기") == 0) {
					Sleep(20);
					SendMenu(clntSock);
					continue;
				}
				char author[50];
				strcpy(author, msg);
                // 같은 저자 도서 찾기
				for (int i = 0; i < book; i++) {
					if (_stricmp(author, books[i].author) == 0) {
						char line[512];
						sprintf(line, "%-5d\t%-50s\t%-50s\t%-5.2f\r\n",
							books[i].num,
							books[i].booktitle,
							books[i].author,
							books[i].rating);
						send(clntSock, line, strlen(line), 0);
						check = 1;
					}
				}
                // 도서 찾기 실패
				if (!check) {
					send(clntSock, "해당 저자의 도서를 찾을 수 없습니다.\r\n", strlen("해당 저자의 도서를 찾을 수 없습니다.\r\n"), 0);
				}
			}
			// 잘못된 번호 입력 시
			else {
				send(clntSock, "잘못된 입력입니다. 기능을 종료합니다.\r\n", strlen("잘못된 입력입니다. 기능을 종료합니다.\r\n"), 0);
			}

			Sleep(20);
			SendMenu(clntSock);
			continue;
		}
		// 3. 도서 정보 추가
		else if (strcmp(msg, "3") == 0) {
			loadbooks(books, &book); // 도서 정보 파일 읽기
	        // 도서 권 수가 700권 이상일 시
			if (book >= MAX_BOOKS) {
				send(clntSock, "도서 수가 700권이 되어 추가할 수 없습니다.\r\n", strlen("도서 수가 700권이 되어 추가할 수 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}

			BOOK newBook;

			char tempBuf[BUF_SIZE];

			// 도서 제목 입력
			send(clntSock, "[입력] 도서 제목 (돌아가기 입력 시 돌아감): ", strlen("[입력] 도서 제목 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, tempBuf, BUF_SIZE - 1, 0);
			tempBuf[strLen] = '\0';
			tempBuf[strcspn(tempBuf, "\r\n")] = '\0';
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (strcmp(tempBuf, "돌아가기") == 0) {
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}

			strncpy(newBook.booktitle, tempBuf, sizeof(newBook.booktitle) - 1);
			newBook.booktitle[sizeof(newBook.booktitle) - 1] = '\0';

			// 공백 입력 방지
			int onlySpace = 1;

			for (int i = 0; newBook.booktitle[i]; i++) {
				if (!isspace((unsigned char)newBook.booktitle[i])) {
					onlySpace = 0;
					break;
				}
			}
			if (onlySpace || strlen(newBook.booktitle) == 0) {
				send(clntSock, "도서 제목은 비어있을 수 없습니다.\r\n", strlen("도서 제목은 비어있을 수 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}

			// 저자 입력
			send(clntSock, "[입력] 저자 (돌아가기 입력 시 돌아감): ", strlen("[입력] 저자 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, tempBuf, BUF_SIZE - 1, 0);
			tempBuf[strLen] = '\0';
			tempBuf[strcspn(tempBuf, "\r\n")] = '\0';
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (strcmp(tempBuf, "돌아가기") == 0) {
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			strncpy(newBook.author, tempBuf, sizeof(newBook.author) - 1);
			newBook.author[sizeof(newBook.author) - 1] = '\0';

			// 공백 입력 방지
			onlySpace = 1;

			for (int i = 0; newBook.author[i]; i++) {
				if (!isspace((unsigned char)newBook.author[i])) {
					onlySpace = 0;
					break;
				}
			}
			if (onlySpace || strlen(newBook.author) == 0) {
				send(clntSock, "저자 이름은 비어있을 수 없습니다.\r\n", strlen("저자 이름은 비어있을 수 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}

			// 중복 도서 검사
			for (int i = 0; i < book; i++) {
				if (_stricmp(books[i].booktitle, newBook.booktitle) == 0 &&
					_stricmp(books[i].author, newBook.author) == 0) {
					send(clntSock, "동일한 책 제목과 저자의 도서가 이미 존재합니다.\r\n", 
						strlen("동일한 책 제목과 저자의 도서가 이미 존재합니다.\r\n"), 0);
					Sleep(20);
					SendMenu(clntSock);
					continue;
				}
			}

			// 평점 입력
			send(clntSock, "[입력] 평점 (0.00 ~ 5.00, 99.9 입력 시 돌아감): ", 
				strlen("[입력] 평점 (0.00 ~ 5.00, 99.9 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, tempBuf, BUF_SIZE - 1, 0);
			tempBuf[strLen] = '\0';
			tempBuf[strcspn(tempBuf, "\r\n")] = '\0';
            // "99.9" 입력 시 메뉴로 돌아감
			if (strcmp(tempBuf, "99.9") == 0) {
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			newBook.rating = (float)atof(tempBuf);

			if (newBook.rating < 0.0 || newBook.rating > 5.0) {
				send(clntSock, "유효하지 않은 평점입니다. 0.00 ~ 5.00 사이 값을 입력하세요.\r\n", 
					strlen("유효하지 않은 평점입니다. 0.00 ~ 5.00 사이 값을 입력하세요.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}

			// 마지막 순번 배정
			int maxnum = 0;

			for (int i = 0; i < book; i++) {
				if (books[i].num > maxnum)
					maxnum = books[i].num;
			}
			newBook.num = maxnum + 1;

			// 파일에 저장
			FILE* fp = fopen(FILE_PATH, "a");
			if (fp == NULL) {
				send(clntSock, "파일 저장 중 오류 발생\r\n", strlen("파일 저장 중 오류 발생\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			fprintf(fp, "%d\t%s\t%s\t%.2f\n", newBook.num, newBook.booktitle, newBook.author, newBook.rating);
			fclose(fp);

			// 구조체에 저장
			books[book] = newBook;
			book++;

			send(clntSock, "도서 정보가 추가되었습니다.\r\n", strlen("도서 정보가 추가되었습니다.\r\n"), 0);
			Sleep(20);
			SendMenu(clntSock);
			continue;
		}
		// 4. 도서 정보 삭제
		else if (strcmp(msg, "4") == 0) {
			loadbooks(books, &book); // 도서 정보 파일 읽기

			char delTitle[50], delAuthor[50];
		
			// 도서 제목 입력
			send(clntSock, "[입력] 삭제할 도서의 책 제목 (돌아가기 입력 시 돌아감): ", 
				strlen("[입력] 삭제할 도서의 책 제목 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(msg, "돌아가기") == 0) {
				Sleep(20);	
				SendMenu(clntSock);
				continue;
			}
			strncpy(delTitle, msg, sizeof(delTitle) - 1);
			delTitle[sizeof(delTitle) - 1] = '\0';
		
			// 저자 입력
			send(clntSock, "[입력] 삭제할 도서의 저자 (돌아가기 입력 시 돌아감): ", 
				strlen("[입력] 삭제할 도서의 저자 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);
			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
			// "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(msg, "돌아가기") == 0) {
				Sleep(20);	
				SendMenu(clntSock);
				continue;
			}
			strncpy(delAuthor, msg, sizeof(delAuthor) - 1);
			delAuthor[sizeof(delAuthor) - 1] = '\0';
		
			// 도서 검색 및 삭제
			int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

			for (int i = 0; i < book; i++) {
				if (_stricmp(books[i].booktitle, delTitle) == 0 && _stricmp(books[i].author, delAuthor) == 0) {
					check = 1;
					for (int j = i; j < book - 1; j++) {
						books[j] = books[j + 1];
					}
					book--;
					break;
				}
			}
		
			if (!check) {
				send(clntSock, "해당 도서를 찾을 수 없습니다.\r\n", strlen("해당 도서를 찾을 수 없습니다.\r\n"), 0);
				Sleep(20);	
				SendMenu(clntSock);
				continue;
			}
		
			// 순번 재정렬
			for (int i = 0; i < book; i++) {
				books[i].num = i + 1;
			}
		
			// 파일 다시 저장
			FILE* fp = fopen(FILE_PATH, "w");
			if (fp == NULL) {
				send(clntSock, "파일 저장 중 오류 발생\r\n", strlen("파일 저장 중 오류 발생\r\n"), 0);
				Sleep(20);	
				SendMenu(clntSock);
				continue;
			}
			for (int i = 0; i < book; i++) {
				fprintf(fp, "%d\t%s\t%s\t%.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);
			}
			fclose(fp);
		
			send(clntSock, "도서가 성공적으로 삭제되었습니다.\r\n", strlen("도서가 성공적으로 삭제되었습니다.\r\n"), 0);
			Sleep(20);	
			SendMenu(clntSock);
			continue;
		}
		// 5. 도서 정보 수정
		else if (strcmp(msg, "5") == 0) {
			loadbooks(books, &book); // 도서 정보 파일 읽기

			char title[50], author[50], temp[BUF_SIZE];
		
			// 도서 제목 입력
			send(clntSock, "[입력] 수정할 도서 제목 (돌아가기 입력 시 돌아감): ", 
				strlen("[입력] 수정할 도서 제목 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);

			if (strLen <= 0) {
				break;
			}

			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(msg, "돌아가기") == 0) {
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			strncpy(title, msg, sizeof(title) - 1);
			title[sizeof(title) - 1] = '\0';
		
			// 저자 입력
			send(clntSock, "[입력] 수정할 도서의 저자 (돌아가기 입력 시 돌아감): ", 
				strlen("[입력] 수정할 도서의 저자 (돌아가기 입력 시 돌아감): "), 0);
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);

			if (strLen <= 0) {
				break;
			}

			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(msg, "돌아가기") == 0) {
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			strncpy(author, msg, sizeof(author) - 1);
			author[sizeof(author) - 1] = '\0';
		
			int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

			for (int i = 0; i < book; i++) {
				if (_stricmp(books[i].booktitle, title) == 0 &&
					_stricmp(books[i].author, author) == 0) {
					check = 1;
		
					char info[256];
					sprintf(info, "\r\n[현재 도서 정보]\r\n제목: %s\r\n저자: %s\r\n평점: %.2f\r\n", books[i].booktitle, books[i].author, books[i].rating);
					send(clntSock, info, strlen(info), 0);
		
					// 새 제목 입력
					send(clntSock, "[입력] 새 도서 제목 (엔터 시 유지, '돌아가기' 입력 시 돌아감): ", 
						strlen("[입력] 새 도서 제목 (엔터 시 유지, '돌아가기' 입력 시 돌아감): "), 0);
					strLen = recv(clntSock, temp, sizeof(temp) - 1, 0);

					if (strLen <= 0) {
						break;
					}

					temp[strLen] = '\0';
					temp[strcspn(temp, "\r\n")] = '\0';
                    // "돌아가기" 입력 시 메뉴로 돌아감
					if (_stricmp(temp, "돌아가기") == 0) {
						Sleep(20);
						SendMenu(clntSock);
						continue;
					}
					if (strlen(temp) > 0 && strcmp(temp, "<ENTER>") != 0)
						strncpy(books[i].booktitle, temp, sizeof(books[i].booktitle) - 1);
		
					// 새 저자 입력
					send(clntSock, "[입력] 새 저자 (엔터 시 유지, '돌아가기' 입력 시 돌아감): ", 
						strlen("[입력] 새 저자 (엔터 시 유지, '돌아가기' 입력 시 돌아감): "), 0);
					strLen = recv(clntSock, temp, sizeof(temp) - 1, 0);

					if (strLen <= 0) {
						break;
					}

					temp[strLen] = '\0';
					temp[strcspn(temp, "\r\n")] = '\0';
                    // "돌아가기" 입력 시 메뉴로 돌아감
					if (_stricmp(temp, "돌아가기") == 0) {
						Sleep(20);
						SendMenu(clntSock);
						continue;
					}
					if (strlen(temp) > 0 && strcmp(temp, "<ENTER>") != 0)
						strncpy(books[i].author, temp, sizeof(books[i].author) - 1);
		
					// 새 평점 입력
					send(clntSock, "[입력] 새 평점 (0.00 ~ 5.00, '99.9' 입력 시 돌아감, 엔터 시 유지): ", 
						strlen("[입력] 새 평점 (0.00 ~ 5.00, '99.9' 입력 시 돌아감, 엔터 시 유지): "), 0);
					strLen = recv(clntSock, temp, sizeof(temp) - 1, 0);

					if (strLen <= 0) {
						break;
					}

					temp[strLen] = '\0';
					temp[strcspn(temp, "\r\n")] = '\0';

					if (strcmp(temp, "99.9") == 0) {
						Sleep(20);
						SendMenu(clntSock);
						continue;
					}
					if (strlen(temp) > 0 && strcmp(temp, "<ENTER>") != 0)
						books[i].rating = atof(temp);
		
					break;
				}
			}
		
			if (!check) {
				send(clntSock, "해당 도서를 찾을 수 없습니다.\r\n", strlen("해당 도서를 찾을 수 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
		
			FILE* fp = fopen(FILE_PATH, "w");
			if (fp == NULL) {
				send(clntSock, "파일 저장 중 오류 발생\r\n", strlen("파일 저장 중 오류 발생\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
			for (int i = 0; i < book; i++) {
				fprintf(fp, "%d\t%s\t%s\t%.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);
			}
			fclose(fp);
		
			send(clntSock, "도서 정보가 성공적으로 수정되었습니다.\r\n", strlen("도서 정보가 성공적으로 수정되었습니다.\r\n"), 0);
			Sleep(20);
			SendMenu(clntSock);
			continue;
		}
		
		// 6. 평점 순 랭킹
		else if (strcmp(msg, "6") == 0) {
			loadbooks(books, &book); // 도서 정보 파일 읽기
		
			if (book == 0) {
				send(clntSock, "출력할 도서가 없습니다.\r\n", strlen("출력할 도서가 없습니다.\r\n"), 0);
				Sleep(20);
				SendMenu(clntSock);
				continue;
			}
		
			BOOK temp[MAX_BOOKS];
			memcpy(temp, books, sizeof(BOOK) * book);
			qsort(temp, book, sizeof(BOOK), ratingbooks);
		
			send(clntSock, "[입력] 엔터를 누르면 평점 순 랭킹이 출력됩니다.\r\n", 
				strlen("[입력] 엔터를 누르면 평점 순 랭킹이 출력됩니다.\r\n"), 0);
		
			strLen = recv(clntSock, msg, BUF_SIZE - 1, 0);

			if (strLen <= 0) {
				break;
			}

			msg[strLen] = '\0';
			msg[strcspn(msg, "\r\n")] = '\0';
		
			for (int i = 0; i < book; i++) {
				char line[512];
				sprintf(line, "%-5d\t%-50s\t%-50s\t%-5.2f\r\n",
					i + 1, temp[i].booktitle, temp[i].author, temp[i].rating);
				if (send(clntSock, line, strlen(line), 0) == SOCKET_ERROR) {
					printf("[서버] send 오류: %d번째 도서 전송 실패\n", i + 1);
					break;
				}
			}
		
			Sleep(20);
			SendMenu(clntSock);
			continue;
		}		
		
		// 잘못된 입력 처리
		else {
			send(clntSock,
				"잘못된 입력입니다. (1~7 사이 번호 입력)\r\n", strlen("잘못된 입력입니다. (1~7 사이 번호 입력)\r\n"), 0);
			Sleep(20);
			SendMenu(clntSock);
			continue;
		}
	}
	closesocket(clntSock);

	return 0;
}

// 클라이언트 연결 수락 함수
unsigned WINAPI SERVER(void* arg) {
    WSADATA wsaData;
    SOCKET servSock, clntSock;
    SOCKADDR_IN servAddr, clntAddr;
    int clntAddrSize;
    HANDLE hThread;

    char port[100] = "55555"; // 기본 포트 번호

    // 윈도우 소켓 라이브러리 초기화
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        ErrorHandling("WSAStartup() error");
    }
    // 뮤텍스 생성
    hmutex = CreateMutex(NULL, FALSE, NULL);
    // 서보 소켓 생성
    servSock = socket(PF_INET, SOCK_STREAM, 0);
    if (servSock == INVALID_SOCKET) {
        ErrorHandling("socket() error");
    }
    // 서버 주소 설정
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(atoi(port));
    // 서보 소켓 활성화
    if (bind(servSock, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR) {
        ErrorHandling("bind() error");
    }

    serversocket = servSock;
    // 클라이언트 연결 대기
    if (listen(servSock, 5) == SOCKET_ERROR) {
        ErrorHandling("listen() error");
    }
    // 클라이언트 접속 수락
    while (1) {
        clntAddrSize = sizeof(clntAddr);
        clntSock = accept(servSock, (SOCKADDR*)&clntAddr, &clntAddrSize);

        if (clntSock == INVALID_SOCKET) {
            ErrorHandling("accept() error");
        }
        // 정보 출력
        printf("\n[서버 알림] 클라이언트 접속: IP = %s, PORT = %d\n",
               inet_ntoa(clntAddr.sin_addr), ntohs(clntAddr.sin_port));

        WaitForSingleObject(hmutex, INFINITE);
        clientsocks[clientcount++] = clntSock;
        ReleaseMutex(hmutex);

        hThread = (HANDLE)_beginthreadex(NULL, 0, HandleClient, (void*)&clntSock, 0, NULL);
        if (hThread == 0) {
            ErrorHandling("_beginthreadex() error");
        }
        CloseHandle(hThread);
    }
    // 서버 종료
    closesocket(servSock);
    WSACleanup();

    return 0;
}
// C:\Temp 폴더 없으면 생성
void TempFolder() {
	_mkdir("C:\\Temp");
}

int main() {
	SetConsoleCP(65001);       // 콘솔 입력 인코딩을 UTF-8로 설정
    SetConsoleOutputCP(65001); // 콘솔 출력 인코딩을 UTF-8로 설정

	TempFolder();

	HANDLE sThread;

	printf("\n-------------------------------------------\n");
	printf("서버가 55555 포트에서 실행 중입니다...\n");
	printf("클라이언트 접속 대기 중...\n");
	printf("-------------------------------------------\n\n");
    // 서버 스레드 실행
	sThread = (HANDLE)_beginthreadex(NULL, 0, SERVER, NULL, 0, NULL);
    if (sThread == 0) {
        ErrorHandling("_beginthreadex() error");
	}

	int choice;

	int login = 0;

	char inputID[15], inputPW[15];

	loadusers(users, &user); // 사용자 정보 파일 읽기
    // 관리자 로그인
	while (!login) {
		printf("\n------- 관리자 로그인 -------\n");
		printf("ID : ");
		scanf("%s", inputID);

		printf("Password : ");
		scanf("%s", inputPW);

		while (getchar() != '\n'); // 입력 버퍼 클리어
        // 입력된 ID와 PW와 사용자 정보 비교
		for (int i = 0; i < user; i++) {
			if (strcmp(users[i].id, inputID) == 0 && strcmp(users[i].pw, inputPW) == 0) {
				login = 1;
				printf("\n로그인 성공. 관리자 모드로 진입합니다.\n");
				break;
			}
		}
        // 로그인 실패 시
		if (!login) {
			printf("\n로그인 실패. 다시 시도하세요.\n");
		}
	}
    // 관리자 메뉴 출력
	while (1) {
		printf("\n");
		printf("----------------- 관리자 -----------------\n");
		printf("--------- 도서 정보 관리 프로그램 ---------\n");
		printf("-------------------------------------------\n");
		printf("메뉴를 선택하세요\n");
		printf("-------------------------------------------\n");
		printf("1. 도서 정보 목록 보기\n");
		printf("2. 도서 정보 검색\n");
		printf("3. 도서 정보 추가\n");
		printf("4. 도서 정보 삭제\n");
		printf("5. 도서 정보 수정\n");
		printf("6. 평점 순 랭킹\n");
		printf("7. 사용자 정보 목록 보기\n");
		printf("8. 사용자 정보 추가\n");
		printf("9. 사용자 정보 삭제\n");
		printf("10. 사용자 아이디/패스워드 수정\n");
		printf("11. 서버 종료\n");
		printf("-------------------------------------------\n");
		printf("사용할 기능의 번호 : ");

		scanf("%d", &choice);

		while (getchar() != '\n');

		if (choice == 1) {
			printf("\n");
			booklist(books, &book);
			printf("\n");
		}
		else if (choice == 2) {
			printf("\n");
			searchbook(books, &book);
			printf("\n");
		}
		else if (choice == 3) {
			printf("\n");
			addbook(books, &book);
			printf("\n");
		}
		else if (choice == 4) {
			printf("\n");
			deletebook(books, &book);
			printf("\n");
		}
		else if (choice == 5) {
			printf("\n");
			modifybook(books, &book);
			printf("\n");
		}
		else if (choice == 6) {
			printf("\n");
			rankbooks(books, &book);
			printf("\n");
		}
		else if (choice == 7) {
			printf("\n");
			userlist(users, &user);
			printf("\n");
		}
		else if (choice == 8) {
			printf("\n");
			adduser(users, &user);
			printf("\n");
		}
		else if (choice == 9) {
			printf("\n");
			deleteuser(users, &user);
			printf("\n");
		}
		else if (choice == 10) {
			printf("\n");
			modifyuser(users, &user);
			printf("\n");
		}
		else if (choice == 11) {
			int exit;

			printf("\n정말 종료하시겠습니까? (1 : 종료  0 : 메뉴) : ");

			scanf("%d", &exit);

			if (exit == 1) {
				printf("\n------------- 서버 종료 -------------\n");

				for (int i = 0; i < clientcount; i++) { 
					closesocket(clientsocks[i]); 
				}
				
				closesocket(serversocket);

				WSACleanup();

				printf("모든 연결을 종료했습니다. 프로그램을 종료합니다.");

				return 0;
			}
			else {
				printf("\n메뉴로 돌아갑니다.\n");
			}

		}
		else {
			printf("\n");
			printf("번호를 다시 입력하세요.");
			printf("\n");
		}
	}

	WaitForSingleObject(sThread, INFINITE);
    CloseHandle(sThread);

	return 0;
}
// 도서 정보 파일 읽기
void loadbooks(BOOK* books, int* book) {
	FILE* fp = fopen(FILE_PATH, "r");
	if (fp == NULL) {
		printf("파일 열기에 실패했습니다.\n");

		*book = 0;

		return;
	}

	*book = 0;

	char line[701];
    // "\t"를 구분자로 분리
	while (fgets(line, sizeof(line), fp)) {
		// 순번
		char* token = strtok(line, "\t");
		if (token != NULL) {
			books[*book].num = atoi(token);
		}
        // 제목
		token = strtok(NULL, "\t");
		if (token != NULL) {
			strcpy(books[*book].booktitle, token);
		}
        // 저자
		token = strtok(NULL, "\t");
		if (token != NULL) {
			strcpy(books[*book].author, token);
		}
        // 평점
		token = strtok(NULL, "\t");
		if (token != NULL) {
			books[*book].rating = atof(token);
		}
        // 개행 문자 제거
		books[*book].booktitle[strcspn(books[*book].booktitle, "\n")] = 0;
		books[*book].author[strcspn(books[*book].author, "\n")] = 0;

		(*book)++;
        
		if (*book >= MAX_BOOKS) {
			break;
		}
	}

	fclose(fp);
}
// 도서 목록 출력
void booklist(BOOK* books, int* book) {
	loadbooks(books, book); // 도서 정보 파일 읽기

	int min, max;

	printf("도서 출력 범위 지정(현재 도서 수 : %d) : ", *book);
    // 출력 범위 입력
	scanf("%d %d", &min, &max);

	while (getchar() != '\n');

	if (min < 1 || max > *book || min > max) {
        printf("잘못된 범위입니다. 유효한 숫자 2개를 입력하세요.\n");

        return;
    }

	for (int i = min - 1; i < max; i++) {
		printf("%-5d\t%-50s\t%-50s\t%-5.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);
	}
}
// 도서 검색
void searchbook(BOOK* books, int* book) {
	loadbooks(books, book); // 도서 정보 파일 읽기

	int choice;
	int booknum;
	int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

	char booktitle[50];
	char author[50];

	printf("\n");
	printf("------------- 도서 정보 검색 -------------\n");
	printf("------------------------------------------\n");
	printf("메뉴를 선택하세요.\n");
	printf("------------------------------------------\n");
	printf("1. 순번으로 검색\n");
	printf("2. 도서 제목으로 검색\n");
	printf("3. 저저로 검색\n");
	printf("------------------------------------------\n");
	printf("방법 선택 : ");

	scanf("%d", &choice);

	while (getchar() != '\n');


	if (choice == 1) {
		printf("도서의 순번 (메뉴로 돌아가려면 '0' 입력) : ");

		scanf("%d", &booknum);

		while (getchar() != '\n');

		if (booknum == 0) {
			printf("\n메뉴로 돌아갑니다.\n");
			return;
		}

		for (int i = 0; i < *book; i++) {
			if (books[i].num == booknum) {
				printf("%-5d\t%-50s\t%-50s\t%-5.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);

				check = 1;

				break;
			}
		}

		if (check == 0) {
			printf("\n해당 순번의 도서를 찾을 수 없습니다.\n");
		}
	}
	else if (choice == 2) {
		printf("도서 제목을 입력하세요 (메뉴로 돌아가려면 '돌아가기' 입력): ");

		fgets(booktitle, sizeof(booktitle), stdin);

		booktitle[strcspn(booktitle, "\n")] = 0;
        // "돌아가기" 입력 시 메뉴로 돌아감
		if (_stricmp(booktitle, "돌아가기") == 0) {
			printf("\n메뉴로 돌아갑니다.\n");
			return;
		}

		for (int i = 0; i < *book; i++) {
			if (_stricmp(booktitle, books[i].booktitle) == 0) {
				printf("%-5d\t%-50s\t%-50s\t%-5.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);

				check = 1;

				break;
			}
		}

		if (check == 0) {
			printf("\n해당 제목을 가진 도서를 찾을 수 없습니다.\n");
		}
	}
	else if (choice == 3) {
    	printf("저자를 입력하세요 (메뉴로 돌아가려면 '돌아가기' 입력): ");
    
		fgets(author, sizeof(author), stdin);

		author[strcspn(author, "\n")] = 0;
        // "돌아가기" 입력 시 메뉴로 돌아감
   
		if (_stricmp(author, "돌아가기") == 0) {
			printf("\n메뉴로 돌아갑니다.\n");
			return;
		}

		int check = 0;
    
		for (int i = 0; i < *book; i++) {
			if (_stricmp(author, books[i].author) == 0) {
				printf("%-5d\t%-50s\t%-50s\t%-5.2f\n",
					books[i].num, books[i].booktitle, books[i].author, books[i].rating);
					check = 1;
				}
			}

			if (check == 0) {
				printf("\n해당 저자를 가진 도서를 찾을 수 없습니다.\n");
			}
		}
		else {
			printf("\n잘못된 입력입니다. 메뉴로 돌아갑니다.\n");
		}
	}
// 도서 추가
void addbook(BOOK* books, int* book) {
	loadbooks(books, book); // 도서 정보 파일 읽기

	if (*book >= MAX_BOOKS) {
		printf("도서 수가 700권이 되어 추가할 수 없습니다.\n");
		return;
	}

	BOOK newbook;

	char ratingstr[10];

	printf("\n------------- 도서 정보 추가 -------------\n");

	printf("도서 제목(메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(newbook.booktitle, sizeof(newbook.booktitle), stdin);

	newbook.booktitle[strcspn(newbook.booktitle, "\n")] = '\0';
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (strcmp(newbook.booktitle, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");

		return;
	}

	// 공백 입력 방지
	int onlySpace = 1;

	for (int i = 0; newbook.booktitle[i]; i++) {
		if (!isspace((unsigned char)newbook.booktitle[i])) {
			onlySpace = 0;
			break;
		}
	}
	if (onlySpace || strlen(newbook.booktitle) == 0) {
		printf("\n도서 제목은 비어있을 수 없습니다.\n");
		return;
	}

	// 저자 입력
	printf("\n저자 이름(메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(newbook.author, sizeof(newbook.author), stdin);

	newbook.author[strcspn(newbook.author, "\n")] = '\0';
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (strcmp(newbook.author, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}
    // 공백 입력 방지
	onlySpace = 1;

	for (int i = 0; newbook.author[i]; i++) {
		if (!isspace((unsigned char)newbook.author[i])) {
			onlySpace = 0;
			break;
		}
	}
	if (onlySpace || strlen(newbook.author) == 0) {
		printf("\n저자 이름은 비어있을 수 없습니다.\n");
		return;
	}

	// 중복 도서 검사
	for (int i = 0; i < *book; i++) {
		if (_stricmp(books[i].booktitle, newbook.booktitle) == 0 &&
			_stricmp(books[i].author, newbook.author) == 0) {
			printf("\n동일한 제목과 저자의 도서가 이미 존재합니다.\n");
			return;
		}
	}

	// 평점 입력
	printf("평점 (0.00 ~ 5.00, (메뉴로 돌아가려면 '99.9' 입력)): ");

	fgets(ratingstr, sizeof(ratingstr), stdin);

	ratingstr[strcspn(ratingstr, "\n")] = '\0';
    // "99.9" 입력 시 메뉴로 돌아감
	if (strcmp(ratingstr, "99.9") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}

	newbook.rating = (float)atof(ratingstr);

	if (newbook.rating < 0.0 || newbook.rating > 5.0) {
		printf("\n유효하지 않은 평점입니다. 0.00 ~ 5.00 사이 값을 입력하세요.\n");
		return;
	}

	// 마지막 순번 배정
	int maxnum = 0;

	for (int i = 0; i < *book; i++) {
		if (books[i].num > maxnum)
			maxnum = books[i].num;
	}

	newbook.num = maxnum + 1;

	// 파일에 저장
	FILE* fp = fopen(FILE_PATH, "a");
	if (fp == NULL) {
		printf("파일 저장 중 오류 발생\n");

		return;
	}
	fprintf(fp, "%d\t%s\t%s\t%.2f\n", newbook.num, newbook.booktitle, newbook.author, newbook.rating);

	fclose(fp);
	// 구조체에 저장
	books[*book] = newbook;

	(*book)++;

	printf("\n도서 정보가 추가되었습니다.\n");
}
// 도서 삭제
void deletebook(BOOK* books, int* book) {
	loadbooks(books, book); // 도서 정보 파일 읽기

	char delTitle[50], delAuthor[50]; 

	int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

	printf("삭제할 도서의 제목을 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(delTitle, sizeof(delTitle), stdin);

	delTitle[strcspn(delTitle, "\n")] = 0;
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(delTitle, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}

	printf("\n삭제할 도서의 저자를 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(delAuthor, sizeof(delAuthor), stdin);

	delAuthor[strcspn(delAuthor, "\n")] = 0;
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(delAuthor, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}

	// 도서 검색 및 삭제
	for (int i = 0; i < *book; i++) {
		if (_stricmp(books[i].booktitle, delTitle) == 0 &&
			_stricmp(books[i].author, delAuthor) == 0) {
			check = 1;

			for (int j = i; j < *book - 1; j++) {
				books[j] = books[j + 1];  // 한 칸씩 당기기
			}
			(*book)--;

			break;
		}
	}

	if (check == 0) {
		printf("\n해당 도서를 찾을 수 없습니다.\n");
		return;
	}

	// 순번 다시 정렬 (1부터 재부여)
	for (int i = 0; i < *book; i++) {
		books[i].num = i + 1;
	}

	// 파일 다시 저장
	FILE* fp = fopen(FILE_PATH, "w");
	if (fp == NULL) {
		printf("파일 저장 중 오류 발생\n");
		return;
	}

	for (int i = 0; i < *book; i++) {
		fprintf(fp, "%d\t%s\t%s\t%.2f\n",
			books[i].num,
			books[i].booktitle,
			books[i].author,
			books[i].rating);
	}

	fclose(fp);

	printf("\n도서가 성공적으로 삭제되었습니다.\n");
}
// 도서 수정
void modifybook(BOOK* books, int* book) {
	loadbooks(books, book);

	char title[50], author[50];
	char temp[50];
	int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

	printf("수정할 도서의 제목을 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(title, sizeof(title), stdin);

	title[strcspn(title, "\n")] = 0;
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(title, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");

		return;
	}

	printf("\n수정할 도서의 저자를 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(author, sizeof(author), stdin);

	author[strcspn(author, "\n")] = 0;
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(author, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");

		return;
	}

	for (int i = 0; i < *book; i++) {
		if (_stricmp(books[i].booktitle, title) == 0 &&
			_stricmp(books[i].author, author) == 0) {

			check = 1;

			printf("\n--------------------- 현재 도서의 정보 ---------------------\n%-50s\t%-50s\t%-5.2f\n\n",
				books[i].booktitle, books[i].author, books[i].rating);

			// 제목 수정
			printf("새 도서 제목 (엔터 시 유지, '돌아가기' 입력 시 메뉴로 이동): ");

			fgets(temp, sizeof(temp), stdin);
			// "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(temp, "돌아가기\n") == 0) {
				printf("\n메뉴로 돌아갑니다.\n");
				return;
			}
			if (temp[0] != '\n') {
				temp[strcspn(temp, "\n")] = 0;
				strcpy(books[i].booktitle, temp);
			}

			// 저자 수정
			printf("\n새 저자 (엔터 시 유지, '돌아가기' 입력 시 메뉴로 이동): ");

			fgets(temp, sizeof(temp), stdin);
			// "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(temp, "돌아가기\n") == 0) {
				printf("\n메뉴로 돌아갑니다.\n");
				return;
			}
			if (temp[0] != '\n') {
				temp[strcspn(temp, "\n")] = 0;
				strcpy(books[i].author, temp);
			}

			// 평점 수정
			printf("\n새 평점 (0.00 ~ 5.00, '99.9' 입력 시 메뉴로 이동, 엔터 시 유지): ");

			fgets(temp, sizeof(temp), stdin);
			// "99.9" 입력 시 메뉴로 돌아감
			if (strcmp(temp, "99.9\n") == 0) {
				printf("\n메뉴로 돌아갑니다.\n");
				return;
			}
			if (temp[0] != '\n') {
				books[i].rating = atof(temp);
			}

			break;
		}
	}

	if (check == 0) {
		printf("\n해당 도서를 찾을 수 없습니다.\n");
		return;
	}

	// 임시 파일에 전체 다시 저장
	FILE* temp_fp = fopen("C:\\Temp\\temp_booklist.txt", "w");
	if (temp_fp == NULL) {
		printf("임시 파일 저장 중 오류 발생\n");
		return;
	}

	for (int i = 0; i < *book; i++) {
		fprintf(temp_fp, "%d\t%s\t%s\t%.2f\n",
			books[i].num,
			books[i].booktitle,
			books[i].author,
			books[i].rating);
	}

	fclose(temp_fp);
	// 기존 파일 삭제 후 임시 파일 이름 변경
	remove(FILE_PATH);

	rename("C:\\Temp\\temp_booklist.txt", FILE_PATH);

	printf("\n도서 정보가 성공적으로 수정되었습니다.\n");
}
// 평점 내림차순 정렬
int ratingbooks(const void* a, const void* b) {
	BOOK* bookA = (BOOK*)a;
	BOOK* bookB = (BOOK*)b;

	if (bookA->rating < bookB->rating) {
		return 1;
	}
	else if (bookA->rating > bookB->rating) {
		return -1;
	}
	else {
		return 0;
	}
}
// 평점 순 랭킹
void rankbooks(BOOK* books, int* book) {
	loadbooks(books, book);

	qsort(books, *book, sizeof(BOOK), ratingbooks);

	for (int i = 0; i < *book; i++) {
		printf("%-5d\t%-50s\t%-50s\t%-5.2f\n", books[i].num, books[i].booktitle, books[i].author, books[i].rating);
	}
}
// 사용자 정보 파일 읽기
void loadusers(USER* users, int* user) {
    FILE* fp = fopen(USER_FILE_PATH, "r");
    if (fp == NULL) {
        printf("파일 열기에 실패했습니다.\n");
        *user = 0;

        return;
    }

    *user = 0;

    char line[100];   

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\n")] = '\0';   

        char* slash = strstr(line, "//");
        if (slash != NULL) {
            *slash = '\0';                  
            slash += 2;                     

            snprintf(users[*user].id, sizeof(users[*user].id), "%s", line);
            snprintf(users[*user].pw, sizeof(users[*user].pw), "%s", slash);

            (*user)++;
            if (*user >= MAX_USERS) break;
        }
    }

    fclose(fp);
}
// 사용자 정보 목록 출력
void userlist(USER* users, int* user) {
	loadusers(users, user); // 사용자 정보 파일 읽기

	if (*user == 0) {
		printf("\n등록된 사용자가 없습니다.\n");
		return;
	}

	printf("\n----------- 사용자 정보(아이디) -----------\n");
	printf("번호\t아이디\t패스워드\n");
	printf("-------------------------------------------\n");

	for (int i = 0; i < *user; i++) {
		printf("%d\t%s\t%s\n", i + 1, users[i].id, users[i].pw);
	}
}
// 사용자 정보 추가
void adduser(USER* users, int* user) {
	loadusers(users, user); // 사용자 정보 파일 읽기

	if (*user >= MAX_USERS) {
		printf("사용자 수가 10명이 되어 추가할 수 없습니다.\n");
		return;
	}

	USER newuser;

	printf("\n------------- 사용자 정보 추가 -------------\n");

	printf("사용자 아이디(메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(newuser.id, sizeof(newuser.id), stdin);

	newuser.id[strcspn(newuser.id, "\n")] = '\0';
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (strcmp(newuser.id, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}

	// 중복 아이디 검사
	for (int i = 0; i < *user; i++) {
		if (_stricmp(users[i].id, newuser.id) == 0) {
			printf("\n동일한 아이디의 사용자가 이미 존재합니다.\n");
			return;
		}
	}

	printf("사용자 패스워드(메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(newuser.pw, sizeof(newuser.pw), stdin);

	newuser.pw[strcspn(newuser.pw, "\n")] = '\0';
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (strcmp(newuser.pw, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}
	
	// 파일에 저장
	FILE* fp = fopen(USER_FILE_PATH, "a");
	if (fp == NULL) {
		printf("파일 저장 중 오류 발생\n");
		return;
	}

	fprintf(fp, "%s//%s\n", newuser.id, newuser.pw);

	fclose(fp);

	// 구조체에 저장
	users[*user] = newuser;

	(*user)++;

	printf("\n사용자 정보가 추가되었습니다.\n");
}
// 사용자 정보 삭제
void deleteuser(USER* users, int* user) {
	loadusers(users, user);

	char deleteid[15];
    char deletepw[15];

	int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

	printf("삭제할 사용자의 아이디를 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(deleteid, sizeof(deleteid), stdin);

	deleteid[strcspn(deleteid, "\n")] = 0;
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(deleteid, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}

	printf("삭제할 사용자의 패스워드를 입력하세요. (메뉴로 돌아가려면 '돌아가기' 입력) : ");

    fgets(deletepw, sizeof(deletepw), stdin);

    deletepw[strcspn(deletepw, "\n")] = 0;

	
	 // ID와 PW 모두 일치해야 삭제 
	 for (int i = 0; i < *user; i++) {
        if (_stricmp(users[i].id, deleteid) == 0 && _stricmp(users[i].pw, deletepw) == 0) {
            // 배열 요소 한 칸씩 앞으로 당김
            for (int j = i; j < *user - 1; j++) {
                users[j] = users[j + 1];
            }
            (*user)--;

            check = 1;
            break;
        }
    }

	if (check == 0) {
		printf("\n해당 아이디와 패스워드드가 일치하는 사용자를 찾을 수 없습니다.\n");
		return;
	}

	// 파일 다시 저장
	FILE* fp = fopen(USER_FILE_PATH, "w");
	if (fp == NULL) {
		printf("파일 저장 중 오류 발생\n");
		return;
	}

	for (int i = 0; i < *user; i++) {
		fprintf(fp, "%s//%s\n", users[i].id, users[i].pw);
	}

	fclose(fp);

	printf("\n사용자 정보가 성공적으로 삭제되었습니다.\n");
}
// 사용자 정보 수정
void modifyuser(USER* users, int* user) {
	loadusers(users, user);

	char modifyid[15];
	char temp[15];

	int check = 0; // 검색 성공 확인 (1이면 성공, 0이면 실패)

	printf("\n------------------ 사용자 정보 추가 ------------------\n");

	printf("수정할 사용자 아이디(메뉴로 돌아가려면 '돌아가기' 입력) : ");

	fgets(modifyid, sizeof(modifyid), stdin);

	modifyid[strcspn(modifyid, "\n")] = '\0';
    // "돌아가기" 입력 시 메뉴로 돌아감
	if (_stricmp(modifyid, "돌아가기") == 0) {
		printf("\n메뉴로 돌아갑니다.\n");
		return;
	}


	for (int i = 0; i < *user; i++) {
		if (_stricmp(users[i].id, modifyid) == 0) {
			check = 1;

			printf("\n--------------- 현재 사용자 정보 ---------------\n");
			printf("ID: %s\nPW: %s\n\n", users[i].id, users[i].pw);

			// 새 ID 입력
			printf("새 ID 입력 (엔터 시 유지, '돌아가기' 입력 시 메뉴로 이동): ");

			fgets(temp, sizeof(temp), stdin);
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(temp, "돌아가기\n") == 0) {
				printf("\n메뉴로 돌아갑니다.\n");

				return;
			}
			if (temp[0] != '\n') {
				temp[strcspn(temp, "\n")] = '\0';

				strcpy(users[i].id, temp);
			}

			// 새 Password 입력
			printf("새 비밀번호 입력 (엔터 시 유지, '돌아가기' 입력 시 메뉴로 이동): ");

			fgets(temp, sizeof(temp), stdin);
            // "돌아가기" 입력 시 메뉴로 돌아감
			if (_stricmp(temp, "돌아가기\n") == 0) {
				printf("\n메뉴로 돌아갑니다.\n");
				return;
			}
			if (temp[0] != '\n') {
				temp[strcspn(temp, "\n")] = '\0';

				strcpy(users[i].pw, temp);
			}

			break;
		}
	}

	if (check == 0) {
		printf("\n해당 ID를 가진 사용자를 찾을 수 없습니다.\n");
		return;
	}

	// 임시 파일에 전체 다시 저장
	FILE* temp_fp = fopen("C:\\Temp\\temp_users.txt", "w");
	if (temp_fp == NULL) {
		printf("임시 파일 저장 중 오류 발생\n");
		return;
	}

	for (int i = 0; i < *user; i++) {
		fprintf(temp_fp, "%s//%s\n", users[i].id, users[i].pw);
	}

	fclose(temp_fp);

	// 기존 파일 삭제 후 임시 파일 교체
	remove(USER_FILE_PATH);

	rename("C:\\Temp\\temp_users.txt", USER_FILE_PATH);

	printf("\n사용자 정보가 성공적으로 수정되었습니다.\n");
}

// 메시지 전송 함수
void SendMsg(char* msg, int len) {
	WaitForSingleObject(hmutex, INFINITE);
	for (int i = 0; i < clientcount; i++) {
		send(clientsocks[i], msg, len, 0);
	}
	ReleaseMutex(hmutex);
}

// 오류 발생 시 메시지를 출력하고 프로그램을 종료하는 함수
void ErrorHandling(char* message) {
	fprintf(stderr, "[ERROR] %s (code: %d)\n", message, WSAGetLastError());
	exit(1);
}
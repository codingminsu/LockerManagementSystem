#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include <time.h>

#define MAXLINE 512 // 문자열의 최대길이
#define MAX_SOCK 128 // 최대 소켓 갯수
 
char *escapechar = "exit"; // 종료할 문자
char name[10];  //채팅에서 사용할 이름

int main(int argc, char *argv[]) {
	char line[MAXLINE], message[MAXLINE + 1]; // 메세지 배열 선언
	char input[MAXLINE + 1] = {0, };
	char ans[MAXLINE], line1[MAXLINE];
  	int n, pid;
  	struct sockaddr_in server_addr; // 서버 소켓 구조체 선언
  	int maxfdp1; // 가장 큰 파일디스크립터 선
	int server_sock;  // 서버와 연결된 소켓번호
	fd_set read_fds; // 읽기위한 파일 디스크립터 선언

	if(argc != 4) {
		printf("사용법 : %s sever_IP port name \n", argv[0]);
		exit(0);
	}

	// 채팅 참가자 이름 구조체 초기화
	sprintf(name, "%s", argv[3]);

	// 소켓 생성
	if ((server_sock = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		printf("Client : Can't open stream socket.\n");
		exit(0);
	}

	// 채팅 서버의 소켓주소 구조체 server_addr 초기화
	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;              
	server_addr.sin_addr.s_addr = inet_addr(argv[1]); 
	server_addr.sin_port = htons(atoi(argv[2]));      

	// 연결요청
	if (connect(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
		printf("Client : Can't connect to server.\n");
		exit(0);
	} else {
		printf("서버에 접속되었습니다. \n");
		// 접속과 동시에 이름 정보를 보낸다.
		int size;
		char temp[32];

		//sprintf(temp, "%s:%s","NAME_INFO", name);
		//if (send(server_sock, temp, strlen(temp), 0) < 0)
		//	printf("Error : Write error on socket.\n");
	}
	// 가장 큰 max 파일 디스크립터 선언
	maxfdp1 = server_sock + 1;
	FD_ZERO(&read_fds);

	while(1) {
		FD_SET(0, &read_fds);
		FD_SET(server_sock, &read_fds);

		if(select(maxfdp1, &read_fds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)  {
			printf("select error\n");
			exit(0);
		}
		// 서버 소켓에서 받은 메세지가 만약에 존재할 경우
		if (FD_ISSET(server_sock, &read_fds))  {
			int size;
			if ((size = recv(server_sock, message, MAXLINE, 0)) > 0)  {
				message[size] = '\0';
				printf("%s\n", message); //우선 화면에 출력
			}
		}

		memset(input, 0, strlen(input));
		// 파일 디스크립터가 0 콘솔이라면
		// 0에 감지가 됬다면
		// 입력이 감지됐다면
		if (FD_ISSET(0, &read_fds)) {
			if (fgets(input, MAXLINE, stdin)) {
				input[strlen(input) - 1] = '\0';
	
				if (strlen(input) > 1) {
					sprintf(line, "%s", input);
					if (send(server_sock, line, strlen(line), 0) < 0) {
						printf("Error: Write error on socket.\n");
					}
					else {
						printf("서버에게 전송완료: %s\n", line);
					}
				}

			}
		}
		
	}  			
}

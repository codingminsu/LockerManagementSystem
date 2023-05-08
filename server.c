#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <signal.h>
#define MAXLINE  512
#define MAX_SOCK 128

// 사물함 최대 갯수 (10)
#define MAX_CABINETS 10
#define STDOUT 1

int i, j, n; // 반복변수
int count=0; // 틀린횟수 세는 변수

int server_sock, client_fd, client_sock;



int maxfdp1;
int num_chat = 0;

int client_s[MAX_SOCK];

char connected_str[128] = "";
char answer[128]="";
fd_set read_fds; // 읽기를 감지할 소켓번호 구조체
struct sockaddr_in client_addr, server_addr;


typedef struct cabinet cabinet;

struct cabinet {
//	int client_info;
	char password[64];
	char contents[128];
	char ans[128];
//	int used;
};

enum {
	NOT_LOGIN,
	LOGIN
};



// 응답받는 문자열
char resp_line[MAXLINE];

// 사물함은 10개 제공
cabinet cabinets[MAX_CABINETS];

/* Define the functions */
void removeClient(int);
int getmax(int);

int main(int argc, char *argv[]) {


	
	pthread_t p_thread[2]; // 스레드의 ID를 저장할 배열

	if((server_sock = socket(PF_INET, SOCK_STREAM, 0)) <0) {
		printf("Server: Can't open strean socket.");
		exit(0);
	}

	bzero((char *)&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(atoi(argv[1]));

	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		printf("Server: Can't bind local address.\n");
		exit(0);
	}

	// 해당 소켓을 통해, 클라이언트로부터 연결요청을 기다린다. (최대 5개)
	listen(server_sock, 5);

	// maxfdp1은 읽기를 감지할 다음 소켓 식별 숫자이다.
	// 다음에 등록한 소켓번호가 maxfdp1이 됨
	maxfdp1 = server_sock + 1 ; // 최대 소켓번호 + 1

	// thr_id = pthread_create(&p_thread[0], NULL,
	

	while(1) {
		FD_ZERO(&read_fds); // 모든 비트 지우기, read_fds를 zero로 만들어버림.
		FD_SET(server_sock, &read_fds); // read_fds 중에서 server_sock에 해당하는 비트를 1로 설정한다.
		// (그 이유는 서버도 데이터를 읽어야해서(메시지를 받아야해서)

		for (i = 0; i < num_chat; i++) {
			FD_SET(client_s[i], &read_fds); // 읽기를 감지할 
		}

		// maxfdp1는 다음 채팅 참가자의 소켓 식별 숫자이다.
		maxfdp1 = getmax(server_sock) + 1; // maxfdp1 재 계산

		if (select(maxfdp1, &read_fds, NULL, NULL, NULL) < 0) {
			printf("select error <= 0 \n");
			exit(0);
		}

		// 접속을 했다면, (server_sock 소켓에 I/O 변화가 발생했다면)
		if (FD_ISSET(server_sock, &read_fds)) {
			time_t timer;
			struct tm *t;
			char timerArr[20];

			client_sock = sizeof(client_addr); // 클라이언트 구조체 크기 저장

			// accept 실행
			client_fd = accept(server_sock, (struct sockaddr *)&client_addr, &client_sock);
			if (client_fd == -1) {
				printf("accept error\n");
				exit(0);
			}

			// 사용자 정보 저장하기
			char tempString[15];
			inet_ntop(AF_INET, &client_addr.sin_addr, tempString, sizeof(tempString)); // ip주소를 바꾼다.

			
			/* 채팅 클라이언트 목록에 추가 */
			client_s[num_chat] = client_fd; // 소켓 정보 저장
			num_chat++;

			char print_str[1024] = "";
			char temp_str[64] = "";
			strcat(print_str, "--------------------\n");
			for (i = 0; i < MAX_CABINETS; i++) {
				// 모든 비밀번호 0000으로 초기화
				sprintf(cabinets[i].password, "%s", "0000");
				sprintf(temp_str, "사물함%d 현재 사용가능\n", i);
				strcat(print_str, temp_str);
			}
			strcat(print_str, "--------------------\n");
			strcat(print_str, "1. 사물함에 내용물 넣기/변경하기 (초기 사물함 비밀번호 0000) \n");
			strcat(print_str, "2. 사물함의 비밀번호 변경하기\n");

			send(client_fd, print_str, strlen(print_str), 0); // 클라이언트에게 연결되었다고,
			// string과 함께 전달.
			printf(" %d번째 사용자 추가.\n", num_chat);

			// 사용자로 부터 받은 1) 입력을 처리하고
			// 그걸 사용자에게 알려준다.
			
		}
		/* 클라이언트가 보낸 메시지가 있는 지 검사한다 */
		for (i = 0; i < num_chat; i++) {
			// 파일디스크립터가 셋팅이 되어있는 지 체크 
			// 2번째 인자에 1번째인자 (fd)가 포함되어있는지 확인한다.
			// 어느 클라이언트에게 I/O가 감지되었는지 그게 참이라면 알개 동작
			if (FD_ISSET(client_s[i], &read_fds)) {
				// recv를 해서 받은 내용이 있는지 검사한다.
				// 0보다 작으면 지나친다. 이상한 내용이니까
				// 해당 클라이언트를 제거
				memset(resp_line, 0, MAXLINE - 1);
				n = recv(client_s[i], resp_line, MAXLINE, 0);
				if (n <= 0) {
					printf("remove client\n");
					removeClient(i); // abrupt exit
					continue;
				}
				printf("%d의 길이만큼  %s 수신 완료 \n", n, resp_line);
				// 처음에 연결을 시도하자마자 클라이언트 정보(명령어)를 보낼 것이다.
				// 그 부분을 감지하여 서버가 처리
				char *pos;
				// '/'의 문자를 파싱해서 처리해야함.
				
				char* strings[10];
				int cnt = 0;
				char* ptr = strtok(resp_line, ".");
				while (ptr != NULL) {
					strings[cnt] = strdup(ptr);
					
					cnt++;
					
					ptr = strtok(NULL, ".");
				}
				
				int changed = 0;
				if (strcmp(strings[0], "1") == 0) {
					char print_str[1024] = "";
					char cabinet_number[64]="";
					char print_str1[1024] = "";
					char temp_str1[64] = "";
					char input_password[128]="";
					int number;
					char print_str2[1024] = "";
					char temp_str2[64] = "";
					char* a[10];
					char print_str3[1024]="";
					char temp_str3[64]="";
					char print_str4[1024]="";
					char print_str5[1024]="";
					char contents[1024] = "";	
					strcat(print_str, "사물함 번호를 입력해주세요. \n");
					send(client_fd, print_str, strlen(print_str), 0);
					recv(client_s[i], cabinet_number, 64, 0);
					number = atoi(cabinet_number);
					if(number >= MAX_CABINETS) {
						printf("%d번 사물함은 존재하지 않습니다.\n", number);
						sprintf(temp_str1,"%d번 사물함은 존재하지 않습니다.\n",number);
						strcat(print_str1,temp_str1);
						send(client_fd, print_str1, strlen(print_str1), 0);
					} 
					else { 
						printf("NUMBER : %d\n",number);
						printf("%s\n",cabinet_number);
						strcat(print_str1, "비밀번호를 입력해주세요(초기 비밀번호 : 0000)\n");
						strcat(print_str1, "비밀번호분실 시 대비 답변을 입력해주세요.(나의 최애곡은?)(예 : 0000/486)\n");
						send(client_fd , print_str1,strlen(print_str1), 0);
						recv(client_s[i], input_password, 128, 0);
						char * ptr = strtok(input_password,"/");
						int cnt = 0;
						while(ptr!=NULL){
							a[cnt]=strdup(ptr);
							ptr=strtok(NULL,"/");
							cnt++;
						}
						strcpy(cabinets[number].ans, a[1]);
							
						if (strcmp(cabinets[number].password,a[0]) == 0) {
							count=0;
							strcat(print_str2, "내용물을 입력하세요\n");
							send(client_fd, print_str2, strlen(print_str2), 0);
							recv(client_s[i], contents, 1024, 0);
							printf("%d번 사물함에 %s 내용물을 넣습니다. \n", number, contents );
							sprintf(temp_str1,"%d번 사물함에 넣었습니다.\n",number);
							strcpy(cabinets[number].contents, contents);
							changed = 1;
							strcat(print_str2,temp_str1);
							send(client_fd, print_str3, strlen(print_str3), 0);
							
						} 
						else {
							strcat(print_str2, "올바르지 않은 비밀번호를 입력하셨습니다.\n");
							count++;
							sprintf(temp_str1, "3회 중 %d회 틀렸습니다.\n",count);
							strcat(print_str2,temp_str1); 
							send(client_fd, print_str2, strlen(print_str2), 0);
							
							if(count==3) {
								count = 0;
								strcat(print_str3,"비밀번호 분실 대비 답변을 입력해주세요.\n");
								strcat(print_str3,"나의 최애곡은?\n");
								
								
								send(client_fd, print_str3, strlen(print_str3), 0);	
								recv(client_s[i], answer, 1024, 0);							
								if(strcmp(cabinets[number].ans,answer)==0){
									count=0;
									strcat(print_str4, "내용물을 입력하세요\n");
									send(client_fd, print_str4, strlen(print_str4), 0);
									recv(client_s[i],contents,1024,0);
									printf("%d번 사물함에 %s 내용물을 넣습니다. \n", number, contents);
									sprintf(temp_str2,"%d번 사물함에 넣었습니다.\n",number);
									strcpy(cabinets[number].contents, contents);
									
									changed = 1;
									strcat(print_str5,temp_str2);
									send(client_fd, print_str5, strlen(print_str5), 0);
								
								}
								
								else
								{
									strcat(print_str4,"설정한 답변이 아닙니다.\n");
									send(client_fd, print_str4, strlen(print_str4), 0);
								
							
								}
							}
						}
					}
				}
				else if (strcmp(strings[0],"2")==0) {
					char print_str[1024] = "";
					char cabinet_number[64]="";
					int number;
					
					char old_password[128]="";
					char new_password[128]="";
					char final_password[128]="";
					
					char print_str1[1024]="";
					char temp_str1[64]="";
					char print_str2[1024]="";
					char print_str3[1024]="";
					char print_str4[1024]="";
					
					strcat(print_str, "사물함 번호를 입력해주세요. \n");
					send(client_fd, print_str, strlen(print_str), 0);
					recv(client_s[i], cabinet_number, 64, 0);
					number = atoi(cabinet_number);
					
					if (number >= MAX_CABINETS) {
						printf("%d번 사물함은 존재하지 않습니다.\n", number);
						sprintf(temp_str1,"%d번 사물함은 존재하지 않습니다.\n",number);
						strcat(print_str1,temp_str1);
						send(client_fd, print_str1, strlen(print_str1), 0);
					} 
					else {
						
						strcat(print_str1, "기존 비밀번호를 입력해주세요\n");
						send(client_fd , print_str1,strlen(print_str1), 0);
						recv(client_s[i], old_password, 128, 0);
						if (strcmp(cabinets[number].password, old_password)==0){
							strcat(print_str2, "새로운 비밀번호를 입력해주세요\n");
							send(client_fd , print_str2,strlen(print_str2), 0);
							recv(client_s[i], new_password, 128, 0);
							
							strcat(print_str3, "한 번 더 입력해주세요\n");
							send(client_fd , print_str3,strlen(print_str3), 0);
							recv(client_s[i], final_password, 128, 0);
							
							if (strcmp(cabinets[number].password, old_password)==0 && strcmp(new_password,final_password)==0) {
								printf("%d번 사물함의 비밀번호를 변경합니다. old: %s => new: %s\n", number, old_password, new_password);
								strcpy(cabinets[number].password, new_password);
								sprintf(temp_str1,"%d번 사물함의 비밀번호 변경 완료.\n", number);
								strcat(print_str4,temp_str1);
								send(client_fd, print_str4, strlen(print_str4), 0);
								changed = 1;
							} 
							else {
								printf("올바르지 않은 비밀번호를 입력하셨습니다.\n");
								strcat(print_str4,"올바르지 않은 비밀번호를 입력하셨습니다.\n");
								send(client_fd, print_str4, strlen(print_str4), 0);
							}
						
						}
						else {
							printf("올바르지 않은 비밀번호를 입력하셨습니다.\n");
							strcat(print_str2,"올바르지 않은 비밀번호를 입력하셨습니다.\n");
							send(client_fd, print_str2, strlen(print_str2), 0);
						}
						
					}
				}	
				
				if (changed == 1) {
					char print_str[1024] = "";
					char temp_str[64] = "";
					strcat(print_str, "--------------------\n");
					for (i = 0; i < MAX_CABINETS; i++) {
						printf("사물함%d / PW: %s, / Contents: %s / 분실 답변 : %s \n", i, cabinets[i].password, cabinets[i].contents, cabinets[i].ans);
						// sprintf(temp_str, "사물함%d 현재 사용가능\n", i);
						// strcat(print_str, temp_str);
					}
					strcat(print_str, "--------------------\n");
				}
			}
		}
		printf("-----------------------\n\n");
		//usleep(10);
	}
}
int getmax(int k) {
	int max = k;
	int r;
	for (r=0; r < num_chat; r++) {
		if (client_s[r] > max ) max = client_s[r];
	}
	return max;
}


void removeClient(int i) {
	close(client_s[i]);
  	if (i != num_chat - 1) {
		client_s[i] = client_s[num_chat-1];
	}
  	num_chat--;
	printf("채팅 참가자 1명 탈퇴. 현재 참가자 수 = %d\n", num_chat);
}


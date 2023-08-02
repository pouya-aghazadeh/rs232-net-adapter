#include <iostream>
#include <winsock2.h>
#include <thread>
#include <string>
#include "rs232.h"
#include <stdio.h>
#include <windows.h>
#include <iterator>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")


using namespace std;

int gettime()
{
  std::time_t result = std::time(nullptr);
  return result;
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	
	string body = "+100000+";
	
	ShowWindow (GetConsoleWindow(), SW_HIDE);
	thread rs232([&]{
		int q, n,
		cport_nr=2,        /* /dev/ttyS0 (COM1 on windows) */
		bdrate=9600;       /* 9600 baud */
	
		unsigned char buf[4096];
		char mode[]={'8','N','1',0};
		if(RS232_OpenComport(cport_nr, bdrate, mode,0)){
			printf("Can not open comport\n");
			return 0;
		}
		while(true){
			n = RS232_PollComport(cport_nr, buf, 4095);
			if(n > 0){
				buf[n] = 0;   
				for(q=0; q < n; q++){
					if(buf[q] < 48 || buf[q] > 57)
						buf[q] = '+';
				}
				//printf("received %i bytes: %s\n", n, (char *)buf);
				unsigned char* bufChar = (unsigned char*)buf;
				int bufLen = strlen((char*)bufChar);
				if(bufLen==8){
					body = (const char *)buf;
				}	
			}
		}
	});
		
	WSADATA wsa;
	SOCKET master , new_socket , client_socket[30] , s;
	struct sockaddr_in server, address;
	int max_clients = 30 , activity, addrlen, i, valread;
	string message = "\
HTTP/1.1 200 OK\r\n\
Host : http://127.0.0.1:62871\r\n\
content-type: application/json\r\n\
Access-Control-Allow-Credentials : true\r\n\
Access-Control-Allow-Origin : *\r\n\
Access-Control-Allow-Headers: *\r\n\
Access-Control-Allow-Methods: *\r\n\
\r\n\
\r\n";

	int MAXRECV = 1024;
	fd_set readfds;
	char *buffer;
	buffer =  (char*) malloc((MAXRECV + 1) * sizeof(char));
	for(i = 0 ; i < 30;i++)
		client_socket[i] = 0;
	if (WSAStartup(MAKEWORD(2,2),&wsa) != 0){
		printf("failed. error : %d",WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	if((master = socket(AF_INET , SOCK_STREAM , 0 )) == INVALID_SOCKET){
		printf("Could not create socket : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(62871);
	if( bind(master ,(struct sockaddr *)&server , sizeof(server)) == SOCKET_ERROR){
		printf("Bind failed with error code : %d" , WSAGetLastError());
		exit(EXIT_FAILURE);
	}
	listen(master , 3);
	addrlen = sizeof(struct sockaddr_in);
	while(true){
		FD_ZERO(&readfds);
		FD_SET(master, &readfds);
		for (  i = 0 ; i < max_clients ; i++){
			s = client_socket[i];
			if(s > 0)
				FD_SET( s , &readfds);
		}
		activity = select( 0 , &readfds , NULL , NULL , NULL);
		if ( activity == SOCKET_ERROR ){
			printf("select call failed with error code : %d" , WSAGetLastError());
			exit(EXIT_FAILURE);
		}
		if (FD_ISSET(master , &readfds)) {
			
			if ((new_socket = accept(master , (struct sockaddr *)&address, (int *)&addrlen))<0){
				perror("accept");
				exit(EXIT_FAILURE);
			}
			
			if( send(new_socket, message.c_str(), strlen(message.c_str()), 0) != strlen(message.c_str()) ){
				perror("send failed");
			}
			
			for (i = 0; i < max_clients; i++) {
				if (client_socket[i] == 0){
					client_socket[i] = new_socket;
					break;
				}
			}
		}
			
		for (i = 0; i < max_clients; i++){
			s = client_socket[i];          
			if (FD_ISSET( s , &readfds)){
				getpeername(s , (struct sockaddr*)&address , (int*)&addrlen);
				valread = recv( s , buffer, MAXRECV, 0);
				if( valread == SOCKET_ERROR){
					int error_code = WSAGetLastError();
					if(error_code == WSAECONNRESET || error_code == WSAECONNABORTED){
						printf("Host disconnected unexpectedly , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
						closesocket( s );
						client_socket[i] = 0;
					}
					else
						printf("recv failed with error code : %d" , error_code);
				}
				if ( valread == 0){
					closesocket( s );
					client_socket[i] = 0;
				}
				else{	
					string temp ="";
					temp += "{\"data\": \"";
					temp += body;
					temp += "\"}";
					send( s , temp.c_str() , strlen(temp.c_str()) , 0 );
					printf("Sent %s\n", temp.c_str());
					shutdown(s, SD_SEND);
				}
			}
		}
	}
		
	closesocket(s);
	WSACleanup();

	rs232.join();

	return 0;
}



#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <iostream>

#include <fstream> //load file
#include <string>
//#include "stdafx.h"
#include <algorithm>

// used for standard input and output, specifically the printf() function
#include <stdio.h>

// arduino board
#include"SerialPort.h"

//nanoRotator
#include"nanoRotator.h"

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 1
#define DEFAULT_PORT "27015"

char output[MAX_DATA_LENGTH];
char *port = "\\\\.\\COM3";

using namespace std;

void load_client_config(string configName, int* client_num, unsigned int *rot_ang, unsigned int *pattern_num, unsigned int *seq_num)
{
	ifstream in;
	string str;

	in.open(configName);

	while (!in.eof())
	{
		while (getline(in, str))
		{
			string::size_type begin = str.find_first_not_of(" \f\t\v");
			//Skips black lines
			if (begin == string::npos)
				continue;
			string firstWord;
			try {
				firstWord = str.substr(0, str.find(" "));
			}
			catch (std::exception &e) {
				firstWord = str.erase(str.find_first_of(" "), str.find_first_not_of(" "));
			}
			transform(firstWord.begin(), firstWord.end(), firstWord.begin(), ::toupper);

			if (firstWord == "CLIENT_NUMBER")
				*client_num = stoi(str.substr(str.find(" ") + 1, str.length()));
			if (firstWord == "ROT_ANG")
			{
				*rot_ang = stoi(str.substr(str.find(" ") + 1, str.length()));
				//*rot_ang = 360 / rot_ang;
			}
			if (firstWord == "SEQ_NUM")
			{
				*seq_num = stoi(str.substr(str.find(" ") + 1, str.length()));
			}
			if (firstWord == "PATTERN_NUM")
			{
				*pattern_num = stoi(str.substr(str.find(" ") + 1, str.length()));
			}
		}
	}
}

int __cdecl main(void)
{

	char *send_p = "p";
	char *send_e = "e";
	char *send_s = "s";
	char *send_w = "w";


	// connect with arduino board
	SerialPort arduino(port);

	if (arduino.isConnected())
	{
		std::cout << "Arduino Connected!" << std::endl << std::endl;
	}


	// connect with and open nanoRotator
	int serialNo = 40871779;
	char rotSerialNo[16];
	sprintf_s(rotSerialNo, "%d", serialNo);
	int nanoRot_err;

	nanoRot_err = connect_nanoRotator(rotSerialNo);

	if (nanoRot_err == -1)
	{
		std::cout << "nanoRotator connection error!!!" << std::endl;
	}



	////////// load server & client config
	int client_num;
	unsigned int rot_ang;
	unsigned int pattern_num;
	unsigned int seq_num;
	load_client_config("seqnum.ini", &client_num, &rot_ang, &pattern_num, &seq_num);

	unsigned int total_ang = 360 / rot_ang;


	////////// init nanoRotator
	//device unit
	int initPos = 3754629; // Position
	int initVeloc = 37546290; // Velocity
	int initAcc = 18773140; // Acceleration

	nanoRot_err = init_nanoRotator(rotSerialNo, initPos, initVeloc, initAcc);
	if (nanoRot_err == -1)
	{
		std::cout << "nanoRotator int error!!!" << std::endl;
	}
	

	

	////////// *STEP 1* initialize Winsock
	WSADATA wsaData;
	int iResult;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}









	////////// *STEP 2* Creating a Socket for the Server
	struct addrinfo *result = NULL;
	struct addrinfo hints; // might be *ptr is non-need

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for the server to listen for client connections
	SOCKET ServerSocket = INVALID_SOCKET;

	ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ServerSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}










	////////// *STEP 3* Binding a Socket to an IP address & port
	iResult = bind(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}
	
	// the address information returned by the getaddrinfo function is no longer needed
	freeaddrinfo(result);










	////////// *STEP 4* Listening on a Socket
	const int nMaxClients = client_num;
	iResult = listen(ServerSocket, nMaxClients);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ServerSocket);
		WSACleanup();
		return 1;
	}








	////////// *STEP 5* Accepting a Connection
	
	int nClient = 0;
	SOCKET *ClientSocket = new SOCKET[2];

	printf("accepting the client.\n");

	// Accept a client socket
	while (nClient<nMaxClients)
	{
		ClientSocket[nClient] = accept(ServerSocket, NULL, NULL);
		if (ClientSocket[nClient] == INVALID_SOCKET) 
		{
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ServerSocket);
			WSACleanup();
			return 1;
		}
		nClient++;
		printf("client %dth is connected.\n", nClient);
	}

	printf("all clients are connecting with server.\n");
	
	// No longer need server socket
	closesocket(ServerSocket);
	








	////////// *STEP 6* Receiving and Sending Data on the Server
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	
	int init_count = 0;
	int start_count = 0;
	int end_count = 0;
	int exit_flag = 0;
	bool allPatternIsCaptured = false;
	int pattern_id = 0;
	int ang_id = 0;
	// Receive until the client shuts down the connection
	do {
		init_count = 0;
		start_count = 0;
		end_count = 0;

		for (int n = 0; n < nMaxClients; n++)
		{

			iResult = recv(ClientSocket[n], recvbuf, recvbuflen, 0);
			printf("client %d send data with len %d\n", n,iResult);

			if (iResult == 0)
			{
				printf("%d client connection is closing...\n", n);
				exit_flag = 1;
			}
			if (iResult > 0)
			{

				int cmp_1 = strcmp(recvbuf, "=");
				int cmp_2 = strcmp(recvbuf, "s");
				int cmp_3 = strcmp(recvbuf, "d");

				if (cmp_1 == 0)
				{
					init_count++;
					printf("receive %d client init signal.\n", n);
					if (init_count == nMaxClients)
					{
						///////////////////////////////////////
						// set monitor pattern

						// set monitor pattern done
						///////////////////////////////////////

						///////////////////////////////////////
						// set lighting

						// set lighting done
						///////////////////////////////////////


						for (int i = 0; i < nMaxClients; i++)
						{
							iSendResult = send(ClientSocket[i], send_p, (int)strlen(send_p), 0);
							if (iSendResult == SOCKET_ERROR) {
								printf("send failed with error: %d\n", WSAGetLastError());
								closesocket(ClientSocket[i]);
								WSACleanup();
								return 1;
							}
						}
						printf("Send P to all %d capture desktop\n", nMaxClients);
					}
				}
				else if (cmp_2 == 0) // start trigger signal
				{
					start_count++;
					if (start_count == nMaxClients)
					{
						pattern_id++;
						if (pattern_id == pattern_num)
						{
							allPatternIsCaptured = true;
						}
						else
						{
							allPatternIsCaptured = false;
						}


						// start trigger
						// start trigger	
						if (arduino.isConnected()) 
						{
							int frame_number = 2000;
							char arr[6];
							sprintf(arr, "%ld", frame_number);
							int frame_count = strlen(arr);
							char *charArray = new char[2 + frame_count + 1];

							charArray[0] = 's';
							strcat(charArray, ":");
							strcat(charArray, arr);
							charArray[2 + frame_count] = '\n';

							arduino.writeSerialPort(charArray, MAX_DATA_LENGTH);
							arduino.readSerialPort(output, MAX_DATA_LENGTH);
							std::cout << output;
							delete[] charArray;

						}

						printf("everything is done, please push output button\n");
						exit_flag = 1;
					}
				}
				else if (cmp_3 == 0)
				{
					end_count++;
					if (end_count == nMaxClients)
					{
						//stop arduino board
						if (arduino.isConnected()) 
						{

							char* charArray = new char[2];

							charArray[0] = 'e';

							//strcpy(charArray, data.c_str());
							charArray[1] = '\n';

							arduino.writeSerialPort(charArray, MAX_DATA_LENGTH);
							//arduino.readSerialPort(output, MAX_DATA_LENGTH);
							//std::cout << output;
							delete[] charArray;

						}

						// check if need to rotate table?
						//
						if (allPatternIsCaptured)
						{
							// rotate nanoRotater with ang
							rotate_nanoRotator(rotSerialNo, rot_ang);

							// reset pattern to the first
							allPatternIsCaptured = false;
							pattern_id = 0;



							ang_id++;
							if (ang_id == total_ang)
							{
								exit_flag = 1;
							}
						}
					}
				}

			}
			
			
		}

	} while (exit_flag==0);



	std::cin.ignore(); //why read something if you need to ignore it? :)






	////////// *STEP 7* Disconnecting the Server
	// shutdown the send half of the connection since no more data will be sent
	for (int i = 0; i < nMaxClients; i++)
	{
		iResult = shutdown(ClientSocket[i], SD_SEND);
		if (iResult == SOCKET_ERROR) {
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket[i]);
			WSACleanup();
			return 1;
		}
	}
	

	// cleanup
	for (int i = 0; i < nMaxClients; i++)
	{
		closesocket(ClientSocket[i]);
	}
	WSACleanup();

	// stop nanoRotator
	nanoRot_err = stop_nanoRotator(rotSerialNo);

	return 0;
}

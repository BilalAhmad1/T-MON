#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include <vector>
#include <sstream>

#pragma comment (lib, "Ws2_32.lib")
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "1104"


std::string _executeCommand(char *cmd)
{
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

bool _commandMatch(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	if (strcmp(tokens[0].c_str(), "cmstp") == 0 || strcmp(tokens[0].c_str(), "cmstp.exe") == 0
		|| strcmp(tokens[0].c_str(), "ipconfig\n") == 0 || strcmp(tokens[0].c_str(), "bitsadmin") == 0)
	{
		return true;
	}
	return false;
}

int main()
{
	WSADATA wsaData;
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;

	struct addrinfo *result = NULL;
	struct addrinfo hints;

	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	char *buff = ""
		"\n\t\t\t\tTRILLIUM INFORMATION SECURITY\n\n\n"
		"\t\t\t\tChallenge Discription\n"
		"1:\tGain System Access using Connection Manager service profile (CMSTP)\n"
		"2:\tbitsadmin can be used to Download File\n\n";
	send(ClientSocket, buff, strlen(buff) + 1, 0);

	do {
		char *rcvData = new char[recvbuflen + 1];
		memset(rcvData, 0, sizeof(char)* (recvbuflen + 1));
		iResult = recv(ClientSocket, rcvData, recvbuflen, 0);
		if (iResult > 0) {
			rcvData[recvbuflen] = '\0';
			printf("Bytes received: %s\n", rcvData);
			iSendResult = 0;
			std::string command = std::string(rcvData);
			if (_commandMatch(command, ' ') == true)
			{
				if (command.find("/create") == std::string::npos)
				{
					std::string _cmdRes = _executeCommand(rcvData);
					iSendResult = send(ClientSocket, _cmdRes.c_str(), _cmdRes.length(), 0);
				}
				else
				{
					std::string _cmdRes = "Not recognized as internal or external command\n";
					iSendResult = send(ClientSocket, _cmdRes.c_str(), _cmdRes.length(), 0);
				}
			}
			else
			{
				std::string _cmdRes = "Not recognized as internal or external command\n";
				iSendResult = send(ClientSocket, _cmdRes.c_str(), _cmdRes.length(), 0);
			}

			if (iSendResult == SOCKET_ERROR) {
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(ClientSocket);
				WSACleanup();
				main();
			}
		}
		else if (iResult == 0)
			printf("Connection closing...\n");
		else  {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
			WSACleanup();
			main();
		}

	} while (iResult > 0);

	iResult = shutdown(ClientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		WSACleanup();
		main();
	}

	closesocket(ClientSocket);
	WSACleanup();
	return 0;
}
#include "stdafx.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <opencv\cv.h>
#include <opencv2\highgui\highgui.hpp>
#include "EngineTester.h"
#include "OCREngine.h"


using namespace cv;


#define DEFAULT_BUFLEN		102400
#define DEFAULT_RESPBUFLEN	100
#define DEFAULT_PORT		"56789"


void MessageProc(char *message, char *resp)
{
	memset(resp, 0, DEFAULT_RESPBUFLEN);

	// Login message
	if (!*(int *)message)
	{
		return;
	}

	// OCR request message
	if (*(int *)message == htonl(1))
	{
		vector<uchar>	jpg;
		Mat			image;
		BYTE		**ppDic = theApp.m_pDicData;
		BYTE		*pLockSymDic = theApp.m_pLockSymDic;
		OCR_RESULT	result;

		jpg.resize(htonl(*((int *)message+1)));
		memcpy(jpg.data(), ((int *)message+2), htonl(*((int *)message+1)));
		image = imdecode(jpg, CV_LOAD_IMAGE_COLOR);
		OCR(image, &result);

		*(int *)resp = htonl(1);
		*((int *)resp+1) = 0;
		*((int *)resp+2) = htonl(result.string.size() + 1);
		memcpy((char *)((int *)resp+3), result.string.data(), result.string.size());

		return;
	}
}

DWORD WINAPI OCRService(LPVOID lpParam)
{
	WSADATA		wsaData;
	SOCKET		ListenSocket = INVALID_SOCKET,
				ClientSocket = INVALID_SOCKET;
	addrinfo	*result = NULL,
				hints;
	char		*recvbuf = new char[DEFAULT_BUFLEN],
				respbuf[DEFAULT_RESPBUFLEN];
	int			iResult, iSendResult,
				nOffset;
	int			recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		AfxMessageBox(L"WSAStartup failed.");

		return 0;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		AfxMessageBox(L"getaddrinfo failed.");
		WSACleanup();

		return 0;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		AfxMessageBox(L"socket failed.");
		freeaddrinfo(result);
		WSACleanup();

		return 0;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		AfxMessageBox(L"bind failed.");
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();

		return 0;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		AfxMessageBox(L"listen failed.");
		closesocket(ListenSocket);
		WSACleanup();

		return 0;
	}

	while (1)
	{
		// Accept a client socket
		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			AfxMessageBox(L"accept failed.");
			closesocket(ListenSocket);
			WSACleanup();

			return 0;
		}

		// Receive until the peer shuts down the connection
		nOffset = 0;

		do
		{
			iResult = recv(ClientSocket, recvbuf+nOffset, recvbuflen-nOffset, 0);
			if (iResult > 0)
			{
				nOffset += iResult;
				if (htonl(*((int *)recvbuf+1))+8 == nOffset)
				{
					break;
				}
			}
			else if (iResult == 0)
			{
				AfxMessageBox(L"Connection closing...");
			}
			else 
			{
				AfxMessageBox(L"recv failed.");
				closesocket(ClientSocket);
				WSACleanup();

				return 0;
			}
		} while (iResult > 0);

		MessageProc(recvbuf, respbuf);

		// Echo the buffer back to the sender
		iSendResult = send(ClientSocket, respbuf, htonl(*((int *)respbuf+2))+12, 0);
		if (iSendResult == SOCKET_ERROR)
		{
			AfxMessageBox(L"send failed.");
			closesocket(ClientSocket);
			WSACleanup();

			return 0;
		}

		// shutdown the connection since we're done
		iResult = shutdown(ClientSocket, SD_SEND);
		if (iResult == SOCKET_ERROR)
		{
			AfxMessageBox(L"shutdown failed.");
			closesocket(ClientSocket);
			WSACleanup();

			return 0;
		}

		// cleanup
		closesocket(ClientSocket);
	}

	// No longer need server socket
	closesocket(ListenSocket);

	WSACleanup();

	delete []recvbuf;

	return 0;
}

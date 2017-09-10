#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include "SerialClass.h"
#include <time.h>


#pragma comment (lib, "ws2_32.lib")

using namespace std;

#define HOST = "192.186.100.50"
Serial::Serial(const char *portName)
{
	//We're not yet connected
	this->connected = false;

	//Try to connect to the given port throuh CreateFile
	wchar_t wtext[20];
	mbstowcs(wtext, portName, strlen(portName) + 1);
	LPWSTR Lport = wtext;
	this->hSerial = CreateFile(Lport,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	//Check if the connection was successfull
	if (this->hSerial == INVALID_HANDLE_VALUE)
	{
		//If not success full display an Error
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {

			//Print Error if neccessary
			printf("ERROR: Handle was not attached. Reason: %s not available.\n", portName);

		}
		else
		{
			printf("ERROR!!!");
		}
	}
	else
	{
		//If connected we try to set the comm parameters
		DCB dcbSerialParams = { 0 };

		//Try to get the current
		if (!GetCommState(this->hSerial, &dcbSerialParams))
		{
			//If impossible, show an error
			printf("failed to get current serial parameters!");
		}
		else
		{
			//Define serial connection parameters for the arduino board
			dcbSerialParams.BaudRate = CBR_9600;
			dcbSerialParams.ByteSize = 8;
			dcbSerialParams.StopBits = ONESTOPBIT;
			dcbSerialParams.Parity = NOPARITY;
			//Setting the DTR to Control_Enable ensures that the Arduino is properly
			//reset upon establishing a connection
			dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;

			//Set the parameters and check for their proper application
			if (!SetCommState(hSerial, &dcbSerialParams))
			{
				printf("ALERT: Could not set Serial Port parameters");
			}
			else
			{
				//If everything went fine we're connected
				this->connected = true;
				//Flush any remaining characters in the buffers 
				PurgeComm(this->hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
				//We wait 2s as the arduino board will be reseting
				Sleep(ARDUINO_WAIT_TIME);
			}
		}
	}

}

Serial::~Serial()
{
	//Check if we are connected before trying to disconnect
	if (this->connected)
	{
		//We're no longer connected
		this->connected = false;
		//Close the serial handler
		CloseHandle(this->hSerial);
	}
}

int Serial::ReadData(char *buffer, unsigned int nbChar)
{
	//Number of bytes we'll have read
	DWORD bytesRead;
	//Number of bytes we'll really ask to read
	unsigned int toRead;

	//Use the ClearCommError function to get status info on the Serial port
	ClearCommError(this->hSerial, &this->errors, &this->status);

	//Check if there is something to read
	if (this->status.cbInQue>0)
	{
		//If there is we check if there is enough data to read the required number
		//of characters, if not we'll read only the available characters to prevent
		//locking of the application.
		if (this->status.cbInQue>nbChar)
		{
			toRead = nbChar;
		}
		else
		{
			toRead = this->status.cbInQue;
		}

		//Try to read the require number of chars, and return the number of read bytes on success
		if (ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL))
		{
			return bytesRead;
		}

	}

	//If nothing has been read, or that an error was detected return 0
	return 0;

}


bool Serial::WriteData(const char *buffer, unsigned int nbChar)
{
	DWORD bytesSend;

	//Try to write the buffer on the Serial port
	if (!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0))
	{
		//In case it don't work get comm error and return false
		ClearCommError(this->hSerial, &this->errors, &this->status);

		return false;
	}
	else
		return true;
}

bool Serial::IsConnected()
{
	//Simply return the connection status
	return this->connected;
}



void main() {

	time_t rawtime;
	struct tm * timeinfo;
	time  (&rawtime);

	string ipAdresss = "192.168.100.50";
	int port = 23;


	//Initialize arduino
	Serial* SP = new Serial("COM5");
	char incomingData[256] = "";
	int dataLength = 255;
	int readResult = 0;


	// Initialize Winsock
	WSADATA data; 
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		cerr << "can't start winsock, Err #" << wsResult << endl;
		return;
	}

	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		cerr << "Can't create socket" << endl;
		WSACleanup();
		return;
	}
	
	//Fill in a hint structure
	sockaddr_in hint; 
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAdresss.c_str(), &hint.sin_addr);

	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		cerr << "can't connect to server" << endl;
		closesocket(sock);
		WSACleanup();
		return;
	}

	// Do-While loop to send and receive data
	char buf[4096];
	string userInput;
	string login = "pennapps\r\nintegration\r\n";
	string turnon = "#OUTPUT,12,1,100\r\n";
	string turnoff = "#OUTPUT,12,1,0\r\n";
	
	

	do {

		// Poll arduino
		readResult = SP->ReadData(incomingData, dataLength);
		incomingData[readResult] = 0;
		for (int i = 0; i < 256; i++) {
			if (incomingData[i] == *"1"){
				int iturnon = send(sock, turnon.c_str(), turnon.size() + 1, 0);
			}
			else if (incomingData[i] == *"0") {
				int iturnoff = send(sock, turnoff.c_str(), turnoff.size() + 1, 0);
			}
		}


		Sleep(500);

		//Prompt the user for some text
	
		int ilogin = send(sock, login.c_str(), login.size() + 1, 0);
		if (ilogin == SOCKET_ERROR) {
			cout << "ERROR" << endl;
			closesocket(sock);
			WSACleanup();
			return;
		}

//		cout << "> ";
//		getline(cin, userInput);
//		if (userInput.size() > 0) {
//			//Send the text
		
//			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
			
			
//		}
		

	} while (true);
	// Close everything
	closesocket(sock);
	WSACleanup();
	return;
}
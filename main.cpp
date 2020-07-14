/************************************************************
*Author:王瑞龙
*E-mail:daisyskye1425@outlook.com
*FileName:main.cpp
*Function:RC522 Information Process Server
*LastChangeData:20200616
************************************************************/

/************************************************************
*包含文件及链接库文件
************************************************************/
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<mysql.h>
#include"config.h"
using namespace std;
#pragma comment(lib,"ws2_32.lib")//link lib
#pragma comment(lib,"libmysql.lib")

/************************************************************
*函数声明
************************************************************/
DWORD WINAPI appProcessThread(LPVOID lpParameter);
DWORD WINAPI rc522ProcessThread(LPVOID lpParameter);

/************************************************************
*全局变量声明
************************************************************/
const int buffLength = 16;
const char serverPort[] = "27015";

WSADATA wsaData;
SOCKET listenSocket = INVALID_SOCKET;

addrinfo* result = NULL;
addrinfo hints;

SOCKET* clientSocketGlobal = NULL;
char securityCode[7];

/************************************************************
*FunctionName:CloseServerSocket
*Function:关闭Socket
************************************************************/
int CloseServerSocket(SOCKET *clientSocket)
{
	int iResult;
	// shutdown the connection
	iResult = shutdown(*clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		cout << "shutdown failed with error:" << WSAGetLastError() << endl;
		closesocket(*clientSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(listenSocket);
	closesocket(*clientSocket);
	WSACleanup();
	return 0;
}
/************************************************************
*ClassName:MysqlDataBase
*Function:操作mysql数据库
************************************************************/
class MysqlDataBase
{
	struct connectInfo
	{
		char* ip;
		int port;
		char* user;
		char* password;
		char* dbName;
	};
	MYSQL mysqlHandler;
	connectInfo info;
	MYSQL_RES* result;
	int resultRowNum;
	MYSQL_ROW row;
public:
	MysqlDataBase(const char* ip, const int port, const char* user, const char* password, const char* dbName)
	{
		mysql_init(&mysqlHandler);//初始化mysql
		this->info.ip = (char*)ip;
		this->info.port = port;
		this->info.user = (char*)user;
		this->info.password = (char*)password;
		this->info.dbName = (char*)dbName;
		this->result = 0;
		this->resultRowNum = 0;
		this->row = 0;
	}
	bool connect()
	{
		if (mysql_real_connect(&mysqlHandler, info.ip, info.user, info.password, info.dbName, info.port, NULL, 0) == 0)
		{
			cout << "connect to mysql database error!" << endl;
			return false;
		}
		cout << "mysql database connect successfully!" << endl;
		return true;
	}
	bool query(const char* query, int mode)//若查询成功返回0，失败返回随机数,mode 0 为插入，1为
	{
		if (mysql_query(&mysqlHandler, query))
		{
			cout << "execute query failed!" << endl;
			return false;
		}
		if (mode == 1)
		{
			result = mysql_store_result(&mysqlHandler);//存储查询结果
			resultRowNum = mysql_num_fields(result);        //将结果集列数存放到num中
		}
		return true;
	}
	int fecthNum()
	{
		if (!result)
		{
			return 0;
		}
		return resultRowNum;
	}
	MYSQL_ROW fecthRow()
	{
		if (!result)
		{
			return 0;
		}
		if (!(row = mysql_fetch_row(result)))
		{
			return 0;
		}
		return row;
	}
	bool freeResult()
	{
		mysql_free_result(result);     //释放结果集所占用的内存
		result = 0;
	}
	bool close()
	{
		mysql_close(&mysqlHandler);
	}
};

MysqlDataBase mysqlDB("127.0.0.1", 3306, "root", "142541", "rc522");
/************************************************************
*FunctionName:main
*Function:程序入口
************************************************************/
int main(int argc, char* argv[]) {
	int iResult;
	int iSendResult;

	cout << "Server starting now..." << endl;
	cout << "connect to mysql database..." << endl;
	if (!mysqlDB.connect())
	{
		return -1;
	}
	cout << "Start to create server socket...." << endl;
	//init winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed with error: " << iResult << endl;
		cin.get();
		cin.get();
		return -1;
	}
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	//resolve the server address and port
	iResult = getaddrinfo(NULL, serverPort, &hints, &result);
	if (iResult != 0)
	{
		cout << "getaddrinfo failed with error: " << iResult << endl;
		WSACleanup();
		cin.get();
		cin.get();
		return -1;
	}

	//create a socket for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);;
	if (listenSocket == INVALID_SOCKET) {
		cout << "socket failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		cin.get();
		cin.get();
		return 1;
	}
	cout << "Create socket successfully!" << endl;
	//setup the tcp listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		cout << "bind failed with error: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	cout << "bind port successfully!" << endl;
	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		cout << "listen failed with error: " << WSAGetLastError() << endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	cout << "listen successfully!" << endl;
	while (true)
	{
		SOCKET *clientSocket = new SOCKET;
		*clientSocket = NULL;
		*clientSocket = accept(listenSocket, 0, 0);
		if (*clientSocket==NULL)
		{
			cout << "Accept failed with error: " << WSAGetLastError() << "\tcoutinue"<<endl;
			closesocket(listenSocket);
			continue;
		}
		cout << "A client connect to this server,clientsocket is:" << *clientSocket << endl;
		char clientType[buffLength] = {0};
		iResult = recv(*clientSocket, clientType, 16, 0);
		if (iResult>0)
		{
			cout << "clientType received: " << iResult << endl;
			cout << "clientType data:" << clientType << endl;
			if (clientType[6] == '1')
			{
				cout << "clientType 1 process:" << endl;
				CreateThread(NULL, 0, &rc522ProcessThread, clientSocket, 0, NULL);
			}
			else if (clientType[6] == '2')
			{
				cout << "clientType 2 process:" << endl;
				CreateThread(NULL, 0, &appProcessThread, clientSocket, 0, NULL);
			}
		}
		else if (iResult == 0)
			cout << "Connection closing..." << endl;
		else {
			cout << "recv failed with error :" << WSAGetLastError() << endl;
			closesocket(*clientSocket);
		}
		
	}


	//CloseServerSocket();
	return 0;
}

DWORD WINAPI rc522ProcessThread(LPVOID lpParameter)
{
	SOCKET* clientSocket = (SOCKET*)lpParameter;
	clientSocketGlobal = clientSocket;
	int iResult = 0;
	int iSendResult = 0;
	char recvBuf[buffLength] = {0};
	char sendBuf[buffLength] = {0};
	do {
		iResult = recv(*clientSocket, recvBuf, 16, 0);
		if (iResult > 0) {
			cout << "Bytes received: " << iResult << endl;
			cout << "receive data:" << recvBuf << endl;

			char cardid[16];
			memset(cardid, 0, sizeof(cardid));

			int i;
			for (i = 0; i < strlen(recvBuf)-2; i++)
			{
				cardid[i] = recvBuf[i + 2];
			}
			cardid[i + 1] = 0;
			cout << "cardid:" << cardid << endl;
			if (recvBuf[0]=='1')//录入模式
			{
				char queStr[256];
				memset(queStr, 0, sizeof(queStr));
				char* str1 = (char*)"insert into cardid (cardid) values (\"";
				char* str2 = (char*)"\");";

				strcat(queStr, str1);
				strcat(queStr, cardid);
				strcat(queStr, str2);

				cout << queStr << endl;

				mysqlDB.query(queStr,0);
				strcpy_s(sendBuf, "mode0state:ok");
			}
			else if (recvBuf[0] == '0')//验证模式
			{
				char queStr[256];
				memset(queStr, 0, sizeof(queStr));
				char* str1 = (char*)"select cardid from cardid where cardid=\"";
				char* str2 = (char*)"\"";
				strcat(queStr, str1);
				strcat(queStr, cardid);
				strcat(queStr, str2);
				mysqlDB.query(queStr,1);
				cout << "num is :" << mysqlDB.fecthNum() << endl;
				MYSQL_ROW row = mysqlDB.fecthRow();
				if (row != nullptr)
				{
					cout << row[0] << endl;
					strcpy_s(sendBuf, "mode1state:ok");
					cout << "ok" << endl;
				}
				else
				{
					strcpy_s(sendBuf, "mode1state:err");
					cout << "error" << endl;
				}
			}

			cout << "send data!" << endl;
			iSendResult = send(*clientSocket, sendBuf, strlen(sendBuf), 0);
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError() << endl;
				closesocket(*clientSocket);
			}
			cout << "Bytes sent: " << iSendResult << endl;
			cout << "data is sent!" << endl;
			memset(recvBuf, 0, sizeof(recvBuf));
		}
		else if (iResult == 0) {
			cout << "Connection closing..." << endl;
			break;
		}
		else {
			cout << "recv failed with error :" << WSAGetLastError() << endl;
			closesocket(*clientSocket);
			break;
		}

	} while (iResult > 0);
	closesocket(*clientSocket);
	free(clientSocket);
	return 0;
}
DWORD WINAPI appProcessThread(LPVOID lpParameter)
{
	SOCKET* clientSocket = (SOCKET*)lpParameter;
	int iResult = 0;
	int iSendResult = 0;
	char recvBuf[buffLength] = { 0 };
	char sendBuf[buffLength] = { 0 };
	/*
	char revUserName[64];
	char revPassword[64];

	recv(*clientSocket, revUserName, 64, 0);

	recv(*clientSocket, revPassword, 64, 0);
	*/
	while (true) {
		iResult = recv(*clientSocket, recvBuf, 16, 0);
		if (iResult > 0) {
			cout << "Bytes received: " << iResult << endl;
			cout << "receive data:" << recvBuf << endl;

			//usertoken 14 
			//return ok
			//code
			//send to rc522
			//char queStr[256];
			//memset(queStr, 0, sizeof(queStr));
			//char* str1 = (char*)"select cardid from userinfo where cardid=\"";
			//char* str2 = (char*)"\"";
			//mysqlDB.query(queStr, 1);
			//cout << "num is :" << mysqlDB.fecthNum() << endl;
			//MYSQL_ROW row = mysqlDB.fecthRow();
			memset(securityCode, 0, sizeof(securityCode));
			strcpy(securityCode, recvBuf);

			cout << "security code sended" << endl;
			char str[16] = "CD";
			strcat(str, securityCode);
			iSendResult = send(*clientSocketGlobal, str, strlen(str), 0);
			if (iSendResult == SOCKET_ERROR) {
				cout << "send failed with error: " << WSAGetLastError() << endl;
				closesocket(*clientSocket);
			}

		}
		else if (iSendResult == 0) {
			cout << "Connection closing..." << endl;
		}
		else {
			cout << "recv failed with error :" << WSAGetLastError() << endl;
			closesocket(*clientSocket);
		}
		memset(recvBuf, 0, sizeof(recvBuf));
	}
	closesocket(*clientSocket);
	free(clientSocket);
	return 0;
}
int appMessagePocess(char* RecvBuf, char* SendBuf)
{
	return 0;
}

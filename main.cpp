/************************************************************
*Author:王瑞龙
*E-mail:daisyskye1425@outlook.com
*FileName:main.cpp
*Function:RC522 Information Process Server
*LastChangeData:20200716
************************************************************/

//TODO
//1.app user login
//2.win platform control program


/************************************************************
*包含文件及链接库文件
************************************************************/
#include<iostream>
#include<WinSock2.h>
#include<WS2tcpip.h>
#include<mysql.h>
#include<string>
#include"config.h"
#include"CSmtp.h"
#include<time.h>

#pragma comment(lib,"ws2_32.lib")//link lib
#pragma comment(lib,"libmysql.lib")

/************************************************************
*函数声明
************************************************************/
DWORD WINAPI appProcessThread(LPVOID lpParameter);
DWORD WINAPI rc522ProcessThread(LPVOID lpParameter);
DWORD WINAPI switchUserType(LPVOID lpParameter);
DWORD WINAPI mailSender(LPVOID lpParameter);
/************************************************************
*全局变量声明
************************************************************/
const int buffLength = 16;
const char serverPort[] = "25500";

WSADATA wsaData;
SOCKET listenSocket = INVALID_SOCKET;

addrinfo* result = NULL;
addrinfo hints;

SOCKET* hardwareClientSocketGlobal = nullptr;
SOCKET* appClientSocketGlobal = nullptr;

char securityCode[7];

/************************************************************
*FunctionName:CloseServerSocket
*Function:关闭Socket
************************************************************/
int CloseServerSocket(SOCKET* clientSocket)
{
	int iResult;
	// shutdown the connection
	iResult = shutdown(*clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		std::cout << "shutdown failed with error:" << WSAGetLastError() << std::endl;
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
			std::cout << "connect to mysql database error!" << std::endl;
			return false;
		}
		std::cout << "mysql database connect successfully!" << std::endl;
		return true;
	}
	bool query(const char* query, int mode)//若查询成功返回0，失败返回随机数,mode 0 为插入，1为查询
	{
		freeResult();
		if (mysql_query(&mysqlHandler, query))
		{
			std::cout << "execute query failed!" << std::endl;
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
		return true;
	}
	bool close()
	{
		mysql_close(&mysqlHandler);
		return true;
	}
};

MysqlDataBase mysqlDB("127.0.0.1", 3306, "root", "446169", "rc522");
/************************************************************
*FunctionName:main
*Function:程序入口
************************************************************/
int main(int argc, char* argv[]) {
	int iResult;
	int iSendResult;

	std::cout << "Server starting now..." << std::endl;
	std::cout << "connect to mysql database..." << std::endl;
	if (!mysqlDB.connect())
	{
		return -1;
	}
	std::cout << "Start to create server socket...." << std::endl;
	//init winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup failed with error: " << iResult << std::endl;
		std::cin.get();
		std::cin.get();
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
		std::cout << "getaddrinfo failed with error: " << iResult << std::endl;
		WSACleanup();
		std::cin.get();
		std::cin.get();
		return -1;
	}

	//create a socket for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);;
	if (listenSocket == INVALID_SOCKET) {
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		std::cin.get();
		std::cin.get();
		return 1;
	}
	std::cout << "Create socket successfully!" << std::endl;
	//setup the tcp listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	std::cout << "bind port successfully!" << std::endl;
	freeaddrinfo(result);

	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}
	std::cout << "listen successfully!" << std::endl;
	int acceptErrorTimes = 0;
	while (true)
	{
		SOCKET* clientSocket = new SOCKET;
		*clientSocket = NULL;
		*clientSocket = accept(listenSocket, 0, 0);
		if (*clientSocket == NULL)
		{
			std::cout << "Accept failed with error: " << WSAGetLastError() << "\tcoutinue" << std::endl;
			closesocket(listenSocket);
			acceptErrorTimes++;
			if (acceptErrorTimes >= 100)
			{
				std::cout << "Accept error too many times, server stop now, please check if there is some error within it!" << std::endl;
				break;
			}
			continue;
		}
		CreateThread(NULL, 0, &switchUserType, clientSocket, 0, NULL);
	}
	CloseServerSocket(&listenSocket);
	return 0;
}

DWORD WINAPI switchUserType(LPVOID lpParameter)
{
	int iResult = 0;
	SOCKET* clientSocket = (SOCKET*)lpParameter;
	std::cout << "A client connect to this server,clientsocket is:" << *clientSocket << std::endl;
	char clientType[buffLength] = { 0 };
	iResult = recv(*clientSocket, clientType, 16, 0);
	if (iResult > 0)
	{
		std::cout << "clientType received: " << iResult << std::endl;
		std::cout << "clientType data:" << clientType << std::endl;
		if (clientType[6] == '1')
		{
			std::cout << "clientType 1 process:" << std::endl;
			CreateThread(NULL, 0, &rc522ProcessThread, clientSocket, 0, NULL);
		}
		else if (clientType[6] == '2')
		{
			std::cout << "clientType 2 process:" << std::endl;
			CreateThread(NULL, 0, &appProcessThread, clientSocket, 0, NULL);
		}
	}
	else if (iResult == 0)
		std::cout << "Connection closing..." << std::endl;
	else {
		std::cout << "recv failed with error :" << WSAGetLastError() << std::endl;
		closesocket(*clientSocket);
	}
	return 0;
}

DWORD WINAPI rc522ProcessThread(LPVOID lpParameter)
{
	SOCKET* clientSocket = (SOCKET*)lpParameter;
	hardwareClientSocketGlobal = clientSocket;
	int iResult = 0;
	int iSendResult = 0;
	char recvBuf[buffLength] = { 0 };
	char sendBuf[buffLength] = { 0 };
	do {
		iResult = recv(*clientSocket, recvBuf, 16, 0);
		if (iResult > 0) {
			std::cout << "Bytes received: " << iResult << std::endl;
			std::cout << "receive data:" << recvBuf << std::endl;

			char cardid[16];
			memset(cardid, 0, sizeof(cardid));

			int i;
			for (i = 0; i < strlen(recvBuf) - 2; i++)
			{
				cardid[i] = recvBuf[i + 2];
			}
			cardid[i + 1] = 0;
			std::cout << "cardid:" << cardid << std::endl;
			if (recvBuf[0] == '1')//录入模式
			{
				char queStr[256];
				//验证是否已经存在此卡号
				memset(queStr, 0, sizeof(queStr));
				char* str1 = (char*)"select cardid from cardid where cardid=\"";
				char* str2 = (char*)"\"";
				strcat(queStr, str1);
				strcat(queStr, cardid);
				strcat(queStr, str2);
				mysqlDB.query(queStr, 1);
				std::cout << "result:" << mysqlDB.fecthNum() << std::endl;
				MYSQL_ROW row = mysqlDB.fecthRow();
				if (row == nullptr)
				{
					//录入卡号数据库
					memset(queStr, 0, sizeof(queStr));
					char* str1 = (char*)"insert into cardid (cardid) values (\"";
					char* str2 = (char*)"\");";
					strcat(queStr, str1);
					strcat(queStr, cardid);
					strcat(queStr, str2);
					std::cout << queStr << std::endl;
					mysqlDB.query(queStr, 0);
					//录入用户数据库
					memset(queStr, 0, sizeof(queStr));
					char* str3 = (char*)"insert into userinfo (cardid,email,password) values (\"";
					char* str4 = (char*)"\",\"null\",\"null\");";
					strcat(queStr, str3);
					strcat(queStr, cardid);
					strcat(queStr, str4);
					std::cout << queStr << std::endl;
					std::cout << queStr << std::endl;
					mysqlDB.query(queStr, 0);

					strcpy_s(sendBuf, "mode0state:ok");
				}
				else
				{
					strcpy_s(sendBuf, "mode0state:err");
				}

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
				mysqlDB.query(queStr, 1);
				std::cout << "num is :" << mysqlDB.fecthNum() << std::endl;
				MYSQL_ROW row = mysqlDB.fecthRow();

				if (row != nullptr)
				{
					std::cout << row[0] << std::endl;
					strcpy_s(sendBuf, "mode1state:ok");
					CreateThread(NULL, 0, &mailSender, cardid, 0, NULL);
					std::cout << "ok" << std::endl;
				}
				else
				{
					strcpy_s(sendBuf, "mode1state:err");
					std::cout << "error" << std::endl;
				}
			}

			std::cout << "send data!" << std::endl;
			iSendResult = send(*clientSocket, sendBuf, strlen(sendBuf), 0);
			if (iSendResult == SOCKET_ERROR) {
				std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
				closesocket(*clientSocket);
			}
			std::cout << "Bytes sent: " << iSendResult << std::endl;
			std::cout << "data is sent!" << std::endl;
			memset(recvBuf, 0, sizeof(recvBuf));
		}
		else if (iResult == 0) {
			std::cout << "Connection closing..." << std::endl;
			break;
		}
		else {
			std::cout << "recv failed with error :" << WSAGetLastError() << std::endl;
			closesocket(*clientSocket);
			break;
		}

	} while (iResult > 0);
	std::cout << "rc522processthreadexit" << endl;
	closesocket(*clientSocket);
	free(clientSocket);
	return 0;
}

DWORD WINAPI appProcessThread(LPVOID lpParameter)
{
	SOCKET* clientSocket = (SOCKET*)lpParameter;
	appClientSocketGlobal = clientSocket;
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

	send(*appClientSocketGlobal, "Hello!", strlen("Hello!"), 0);

		while (true) {
			iResult = recv(*clientSocket, recvBuf, 16, 0);
			if (iResult > 0) {
				std::cout << "Bytes received: " << iResult << std::endl;
				std::cout << "receive data:" << recvBuf << std::endl;

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

				std::cout << "security code sended" << std::endl;
				char str[16] = "CD";
				strcat(str, securityCode);
				std::cout << str << std::endl;
				iSendResult = send(*appClientSocketGlobal, str, strlen(str), 0);
				if (iSendResult == SOCKET_ERROR) {
					std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
					closesocket(*clientSocket);
				}

			}
			else if (iSendResult == 0) {
				std::cout << "Connection closing..." << std::endl;
				break;
			}
			else {
				std::cout << "recv failed with error :" << WSAGetLastError() << std::endl;
				closesocket(*clientSocket);
				break;
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

DWORD WINAPI mailSender(LPVOID lpParameter)
{
	char* cardid = (char*)lpParameter;
	bool bError = false;
	try {
		time_t curtime = time(0);
		tm time = *localtime(&curtime);

		string str;
		str = "解锁时间为:";
		str += to_string(time.tm_year + 1900);
		str += "年";
		str += to_string(time.tm_mon + 1);
		str += "月";
		str += to_string(time.tm_mday);
		str += "日";
		str += to_string(time.tm_hour);
		str += ":";
		str += to_string(time.tm_min);
		str += ":";
		str += to_string(time.tm_sec);
		char* time_char = (char*)str.data();
		std::cout << "time is:" << time_char << std::endl;
		char mailAddr[64];
		char queStr[256];
		memset(queStr, 0, sizeof(queStr));
		char* str1 = (char*)"select cardid,email from userinfo where cardid=\"";
		char* str2 = (char*)"\"";
		strcat(queStr, str1);
		strcat(queStr, cardid);
		strcat(queStr, str2);
		mysqlDB.query(queStr, 1);
		std::cout << "mail:cardid: " << cardid << endl;
		std::cout << "num is :" << mysqlDB.fecthNum() << std::endl;
		MYSQL_ROW row = mysqlDB.fecthRow();
		if (row != nullptr && mysqlDB.fecthNum() == 2)
		{
			std::cout << "row0:" << row[0] << std::endl;
			std::cout << "row1:" << row[1] << std::endl;
			strcpy_s(mailAddr, row[0]);
			std::cout << "mail start to send!" << std::endl;
		}
		else
		{
			return 0;
		}

		CSmtp mail;

		mail.SetSMTPServer("smtp.qq.com", 465);//邮件服务器地址
		mail.SetSecurityType(USE_SSL);//使用SSL

		mail.SetLogin("1175445708@qq.com");//登录账号
		mail.SetPassword("pmsqfjrtnplihbbb");//密码

		mail.SetSenderName("RC522Server");//发件人昵称
		mail.SetSenderMail("1175445708@qq.com");//发件人邮箱
		mail.SetReplyTo("user@domain.com");//回复邮箱

		mail.SetSubject("新的解锁通知");//邮件标题
		mail.AddRecipient(row[1]);//接收者

		mail.SetXPriority(XPRIORITY_NORMAL);//邮件优先级
		mail.SetXMailer("The Bat! (v3.02) Professional");//发送邮件的客户端标识

		//邮件内容
		mail.AddMsgLine("你好，我们发现你使用了你的 IC卡 进行了解锁操作。");
		mail.AddMsgLine("这是你的行为吗？");
		mail.AddMsgLine("如果不是说明你的 IC卡 已经丢失，请及时挂失！");
		mail.AddMsgLine(time_char);
		mail.AddMsgLine("--From RC522 Server");
		mail.AddMsgLine("DO not reply!");
		//mail.AddAttachment("../test1.jpg");
		//mail.AddAttachment("c:\\test2.exe");
		//mail.AddAttachment("c:\\test3.txt");

		mail.Send();//发送邮件
	}
	catch (ECSmtp e)
	{
		std::cout << "Error: " << e.GetErrorText().c_str() << ".\n";
		bError = true;
	}
	if (!bError)
		std::cout << "Mail was send successfully.\n";
	return 1;
}

#include "Asteroid.h"
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <thread>
#include <filesystem>
#include <iostream>

#include <time.h>
#include <atomic>

#pragma comment(lib, "ws2_32.lib")

#define SOCKET_MAX_BUFFER 1048576
#define PROTOCOL_ID 123456789
#define PACKET_SIZE 262144

enum class DataType {
	FUNCTION_BEGIN = 1,
	FUNCTION_END = 2,
	FRAME_END = 3
};

#define SetEnum(val) SetInt((int)val)

class Packet {
public:
	Packet()
	{
		// TODO: no crear i borrar data a cada frame, reciclar el buffer!
		data = new char[maxSize];
		memset(data, '\0', maxSize);
		cursor = data;
		SetInt(PROTOCOL_ID);
		cursor += sizeof(int);
		currentSize += sizeof(int);
	}

	~Packet()
	{
		delete[] data;
	}

	const char* GetData()
	{
		return data;
	}

	void SetDouble(double d)
	{
		size_t size = sizeof(double);
		UpdateSize(size);
		memcpy(cursor, &d, size);
		cursor += size;
	}

	void SetInt(int d)
	{
		size_t size = sizeof(int);
		UpdateSize(size);
		memcpy(cursor, &d, size);
		cursor += size;
	}

	void SetChar(const char* d, size_t s)
	{
		UpdateSize(s);
		memcpy(cursor, d, s);
		cursor += s;
	}

	void SetString(const std::string& str)
	{
		SetInt(str.size());
		SetChar(str.data(), str.size());
	}

	size_t GetSize() const
	{
		return currentSize;
	}

	void WriteCurrentSize()
	{
		memcpy(data + sizeof(int), &currentSize, sizeof(int));
	}

private:

	void UpdateSize(size_t size)
	{
		currentSize += size;
		while (currentSize >= maxSize) {
			size_t newSize = maxSize + PACKET_SIZE;
			char* moreData = new char[newSize];
			memcpy(moreData, data, maxSize);
			maxSize = newSize;
			delete[] data;
			data = moreData;
			cursor = data;
			cursor += (currentSize - size);
		}
	}

private:
	char* data = nullptr;
	char* cursor = nullptr;
	size_t currentSize = 0U;
	size_t maxSize = PACKET_SIZE;
};

bool isConnected = false;
SOCKET client = NULL;
sockaddr_in bindAddr;
Packet* packet = nullptr;
std::thread connectionThread;
std::atomic_bool threadSucced;
std::atomic_bool threadDead;

bool HasSocketInfo(SOCKET socket)
{
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(socket, &readfds);

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	select(0, &readfds, nullptr, nullptr, &timeout);
	return FD_ISSET(socket, &readfds);
}

bool CheckThread()
{
	bool ret = threadSucced.load();
	if (ret) {
		threadDead.store(true);
		connectionThread.join();
	}
	return ret;
}

void TryConnection()
{
	while (!threadDead.load()) {
		if (connect(client, (const sockaddr*)&bindAddr, sizeof(bindAddr)) != SOCKET_ERROR) {
			threadSucced.store(true);
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}
}

void CreateClientSocket()
{
	isConnected = false;
	threadSucced.store(false);
	threadDead.store(false);
	if (client != NULL) {
		closesocket(client);
	}
	client = socket(AF_INET, SOCK_STREAM, 0);
	connectionThread = std::thread(TryConnection);
}

AsteroidFrameData::AsteroidFrameData()
{
	if (isConnected || (isConnected = CheckThread())) {
		if (HasSocketInfo(client)) {
			CreateClientSocket();
		}
		else {
			packet = new Packet();
			clock.Start();
		}
	}
}

AsteroidFrameData::~AsteroidFrameData()
{
	if (isConnected) {
		double ms = clock.ReadMs();

		packet->SetEnum(DataType::FRAME_END);
		packet->SetDouble(ms);

		packet->WriteCurrentSize();

		char* data = (char*)packet->GetData();
		size_t size = packet->GetSize();
		size_t dataSend = 0U;
		while (dataSend != size) {
			size_t toSend = size - dataSend > SOCKET_MAX_BUFFER ? SOCKET_MAX_BUFFER : size - dataSend;
			dataSend += toSend;

			send(client, data, toSend, 0);
			data += toSend;
		}

		delete packet;
		packet = nullptr;
	}
}

void AsteroidInit()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult == NO_ERROR) {
		CreateClientSocket();

		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(8000);
		const char* remoteAddrStr = "127.0.0.1";
		inet_pton(AF_INET, remoteAddrStr, &bindAddr.sin_addr);
	}
}

void AsteroidCleanup()
{
	if (client != NULL) {
		closesocket(client);
	}

	if (!threadSucced.load()) {
		threadDead.store(true);
		connectionThread.join();
	}

	if (packet != nullptr) {
		delete packet;
		packet = nullptr;
	}

	if (WSACleanup() != NO_ERROR) {
		// ERROR
	}
}

AsteroidFunctionData::AsteroidFunctionData(const char* functionName, const char* fileName, int line, int functionSize, int fileSize, AsteroidColor color)
{
	if (isConnected) {
		packet->SetEnum(DataType::FUNCTION_BEGIN);
		packet->SetInt(fileSize);
		packet->SetChar(fileName, fileSize);
		packet->SetInt(functionSize);
		packet->SetChar(functionName, functionSize);
		packet->SetInt(line);
		packet->SetEnum(color);

		clock.Start();
	}
}

AsteroidFunctionData::~AsteroidFunctionData()
{
	if (isConnected) {
		double ms = clock.ReadMs();

		packet->SetEnum(DataType::FUNCTION_END);
		packet->SetDouble(ms);
	}
}

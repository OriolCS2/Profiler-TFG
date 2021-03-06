#include "Packet.h"

Packet::Packet(char* d, size_t size)
{
	buffer = d;
	memcpy(buffer, d, size);
	cursor = buffer;
	capacity = size;
	this->size = 0;
}

Packet::~Packet()
{
	delete[] buffer;
}

void Packet::Read(void* outData, size_t bytes) const
{
	size_t nextSize = size + bytes;
	if (nextSize <= capacity) {
		memcpy(outData, cursor, bytes);
		cursor += bytes;
		size = nextSize;
	}
	else {
		int u = 0;
	}
}

void Packet::ReadString(std::string& inString) const
{
	int elementCount;
	Read(elementCount);
	inString.resize(elementCount);
	for (auto& character : inString) {
		Read(character);
	}
}

void Packet::Read(char* inString)
{
	size_t elementCount;
	Read(elementCount);
	for (size_t i = 0; i < elementCount; ++i) {
		Read(inString[i]);
	}
}

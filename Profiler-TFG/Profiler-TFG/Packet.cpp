#include "Packet.h"

Packet::Packet(size_t size)
{
	buffer = new char[size];
	cursor = buffer;
	capacity = size;
	this->size = 0;
}

Packet::~Packet()
{
	delete[] buffer;
}

uint Packet::GetCapacity() const
{
	return capacity;
}

uint Packet::GetSize() const
{
	return size;
}

char* Packet::GetBufferPtr() const
{
	return buffer;
}

void Packet::Read(void* outData, size_t bytes) const
{
	size_t nextSize = size + bytes;
	if (nextSize < capacity) {
		memcpy(outData, cursor, bytes);
		cursor += bytes;
		size = nextSize;
	}
}

void Packet::Read(std::string& inString)
{
	size_t elementCount;
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
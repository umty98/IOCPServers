//#include "PacketBufferReader.h"
//#include <cstring>
#include "pch.h"

TLSMemoryPool<typename PacketBufferReader> PacketBufferReader::pool;

PacketBufferReader::PacketBufferReader()
	:PacketBufferReader(eBUFFER_DEFAULT) {}

PacketBufferReader::PacketBufferReader(int bufferSize)
	:p_BufferSize(bufferSize), p_DataSize(0), p_ReadPos(0), p_WritePos(0), p_Error(false), resizeCnt(0)
{
	p_Buffer = new char[p_BufferSize];
}

PacketBufferReader::~PacketBufferReader()
{
	delete[] p_Buffer;
}

void PacketBufferReader::Clear()
{
	p_DataSize = 0;
	p_ReadPos = 0;
	p_WritePos = 0;
}

int PacketBufferReader::MoveWritePos(int size)
{
	if (size <0 || p_WritePos + size > p_BufferSize) return 0;

	p_WritePos += size;
	//
	p_DataSize = p_WritePos;
	return size;
}

int PacketBufferReader::MoveReadPos(int size)
{
	if (size <0 || p_ReadPos + size > p_WritePos) return 0;
	p_ReadPos += size;
	return size;
}

bool PacketBufferReader::ResizeBuffer(int newSize)
{
	if (newSize <= 0 || newSize <= p_DataSize)
		return false;

	if (resizeCnt > 2)
		return false;

	char* newBuffer = new char[newSize];
	if (!newBuffer)
		return false;

	resizeCnt++;

	std::memcpy(newBuffer, p_Buffer, p_DataSize);

	delete[] p_Buffer;

	p_Buffer = newBuffer;
	p_BufferSize = newSize;

	//·Î±× Âï±â

	return true;
}

PacketBufferReader& PacketBufferReader::operator>>(unsigned char& value)
{
	if (p_ReadPos + sizeof(unsigned char) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = p_Buffer[p_ReadPos++];
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(char& value)
{
	if (p_ReadPos + sizeof(char) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = p_Buffer[p_ReadPos++];
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(short& value)
{
	if (p_ReadPos + sizeof(short) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(short*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(short);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(unsigned short& value)
{
	if (p_ReadPos + sizeof(unsigned short) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(unsigned short*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(unsigned short);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(int& value)
{
	if (p_ReadPos + sizeof(int) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(int*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(int);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(unsigned int& value)
{
	if (p_ReadPos + sizeof(unsigned int) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(unsigned int*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(unsigned int);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(long& value)
{
	if (p_ReadPos + sizeof(long) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(long*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(long);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(unsigned long& value)
{
	if (p_ReadPos + sizeof(unsigned long) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(unsigned long*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(unsigned long);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(float& value)
{
	if (p_ReadPos + sizeof(float) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(float*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(float);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(double& value)
{
	if (p_ReadPos + sizeof(double) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(double*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(double);
	return *this;
}

PacketBufferReader& PacketBufferReader::operator>>(__int64& value)
{
	if (p_ReadPos + sizeof(__int64) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(__int64*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(__int64);
	return *this;
}

int PacketBufferReader::DequeueData(char* dest, int size)
{
	if (size <= 0 || p_ReadPos + size > p_WritePos) return 0;

	std::memcpy(dest, p_Buffer + p_ReadPos, size);
	p_ReadPos += size;
	return size;
}

int PacketBufferReader::EnqueueData(const char* src, int size)
{
	if (size <= 0 || p_WritePos + size > p_BufferSize) return 0;

	std::memcpy(p_Buffer + p_WritePos, src, size);
	p_WritePos += size;
	p_DataSize = p_WritePos;
	return size;
}

int PacketBufferReader::PeekData(char* dest, int size)
{
	if (size <= 0 || p_ReadPos + size > p_WritePos) return 0;

	std::memcpy(dest, p_Buffer + p_ReadPos, size);

	return size;
}

int PacketBufferReader::copyRest(PacketBufferReader* oldBuf, PacketBufferReader* newBuf)
{
	if (oldBuf->GetDataSize() == 0)
		return 0;

	std::memcpy(newBuf->GetBufferPtr(), oldBuf->GetBufferPtr() + oldBuf->p_ReadPos, oldBuf->GetDataSize());
	newBuf->MoveWritePos(oldBuf->GetDataSize());

	return oldBuf->GetDataSize();
}

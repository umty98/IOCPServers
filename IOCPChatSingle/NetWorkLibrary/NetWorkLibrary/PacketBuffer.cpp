//#include "PacketBuffer.h"
//#include <cstring>
//#include "Protocol.h"
//#include "CLanServer.h"
#include "pch.h"

TLSMemoryPool<typename PacketBuffer> PacketBuffer::pool;

PacketBuffer::PacketBuffer()
	:PacketBuffer(eBUFFER_DEFAULT) {}

PacketBuffer::PacketBuffer(int bufferSize)
	:p_BufferSize(bufferSize), p_DataSize(0), p_ReadPos(0), p_WritePos(0), p_Error(false), resizeCnt(0), refCount(1), isEncode(false)
{
	p_Buffer = new char[p_BufferSize];
}

PacketBuffer::~PacketBuffer()
{
	delete[] p_Buffer;
}

void PacketBuffer::Clear()
{
	p_DataSize = 0;
	p_ReadPos = 0;
	p_WritePos = 0;
	refCount = 1;
	isEncode = false;
}

int PacketBuffer::MoveWritePos(int size)
{
	if (size <0 || p_WritePos + size > p_BufferSize) return 0;

	p_WritePos += size;
	//
	p_DataSize = p_WritePos;
	return size;
}

int PacketBuffer::MoveReadPos(int size)
{
	if (size <0 || p_ReadPos + size > p_WritePos) return 0;
	p_ReadPos += size;
	return size;
}

bool PacketBuffer::ResizeBuffer(int newSize)
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

PacketBuffer& PacketBuffer::operator<<(unsigned char value)
{
	if (p_WritePos + sizeof(unsigned char) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	p_Buffer[p_WritePos++] = value;
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(char value)
{
	if (p_WritePos + sizeof(char) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	p_Buffer[p_WritePos++] = value;
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(short value)
{
	if (p_WritePos + sizeof(short) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(short*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(short);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(unsigned short value)
{
	if (p_WritePos + sizeof(unsigned short) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(unsigned short*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(unsigned short);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(int value)
{
	if (p_WritePos + sizeof(int) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(int*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(int);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(unsigned int value)
{
	if (p_WritePos + sizeof(unsigned int) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(unsigned int*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(unsigned int);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(long value)
{
	if (p_WritePos + sizeof(long) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(long*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(long);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(unsigned long value)
{
	if (p_WritePos + sizeof(unsigned long) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(unsigned long*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(unsigned long);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(float value)
{
	if (p_WritePos + sizeof(float) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(float*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(float);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(double value)
{
	if (p_WritePos + sizeof(double) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(double*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(double);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator<<(unsigned __int64 value)
{
	if (p_WritePos + sizeof(unsigned __int64) > p_BufferSize)
	{
		if (!ResizeBuffer(p_BufferSize * 2))
		{
			p_Error = true;
			return *this;
		}
	}

	*(__int64*)(p_Buffer + p_WritePos) = value;
	p_WritePos += sizeof(__int64);
	p_DataSize = p_WritePos;
	return *this;
}

PacketBuffer& PacketBuffer::operator>>(unsigned char& value)
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

PacketBuffer& PacketBuffer::operator>>(char& value)
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

PacketBuffer& PacketBuffer::operator>>(short& value)
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

PacketBuffer& PacketBuffer::operator>>(unsigned short& value)
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

PacketBuffer& PacketBuffer::operator>>(int& value)
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

PacketBuffer& PacketBuffer::operator>>(unsigned int& value)
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

PacketBuffer& PacketBuffer::operator>>(long& value)
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

PacketBuffer& PacketBuffer::operator>>(unsigned long& value)
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

PacketBuffer& PacketBuffer::operator>>(float& value)
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

PacketBuffer& PacketBuffer::operator>>(double& value)
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

PacketBuffer& PacketBuffer::operator>>(unsigned __int64& value)
{
	if (p_ReadPos + sizeof(unsigned __int64) > p_DataSize)
	{
		value = 0;
		p_Error = true;
		return *this;
	}
	value = *(unsigned __int64*)(p_Buffer + p_ReadPos);
	p_ReadPos += sizeof(unsigned __int64);
	return *this;
}

int PacketBuffer::PeekData(char* dest, int size)
{
	if (size <= 0 || p_ReadPos + size > p_WritePos) return 0;

	std::memcpy(dest, p_Buffer + p_ReadPos, size);
	//p_ReadPos += size;
	return size;
}

int PacketBuffer::EnqueueData(const char* src, int size)
{
	if (size <= 0 || p_WritePos + size > p_BufferSize) return 0;

	std::memcpy(p_Buffer + p_WritePos, src, size);
	p_WritePos += size;
	p_DataSize = p_WritePos;
	return size;
}

int PacketBuffer::EnqueueHeader(const char* src, int size)
{
	std::memcpy(p_Buffer, src, size);

	return size;
}

int PacketBuffer::DequeueData(char* dest, int size)
{
	if (size <= 0 || p_ReadPos + size > p_WritePos) return 0;

	std::memcpy(dest, p_Buffer + p_ReadPos, size);
	p_ReadPos += size;
	return size;
}

int PacketBuffer::EnqueueId(uint64_t sessionId)
{


	return 0;
}

void PacketBuffer::SetCheckSum(PacketBuffer* buffer)
{
	int dataSize = buffer->GetDataSize();
	char* start = buffer->GetBufferPtr();
	char* ptr = start + sizeof(CLanServer::NetWorkHeader);
	char* end = start + dataSize;
	char* check = buffer->GetBufferPtr() + 4;
	BYTE checkSum = 0;

	for (; ptr < end; ++ptr)
	{
		checkSum += *ptr;
	}

	*check = checkSum;
}

int PacketBuffer::GetPayLoadCheckSum(PacketBuffer* buffer)
{
	int dataSize = buffer->GetDataSize();
	char* start = buffer->GetBufferPtr() + sizeof(CLanServer::NetWorkHeader);
	char* ptr = start;
	char* end = start + dataSize;
	BYTE checkSum = 0;

	for (; ptr < end; ++ptr)
	{
		checkSum += *ptr;
	}
	return checkSum;
}

void PacketBuffer::Encode(PacketBuffer* buffer)
{
	if (buffer->GetBufEncoded())
		return;

	buffer->SetBufEncodedTrue();

	//SetCheckSum(buffer);

	BYTE fixedKey = Fixed_Code;
	BYTE randomKey = *(buffer->GetBufferPtr() + 3);
	int dataSize = buffer->GetDataSize();
	char* start = buffer->GetBufferPtr();
	char* ptr = start;
	char* end = start + dataSize;
	ptr += 4;

	BYTE prevP = 0;
	BYTE prevE = 0;
	int  offset = 1;

	for (; ptr < end; ++ptr, ++offset)
	{
		BYTE p = *ptr ^ (prevP + randomKey + offset);

		BYTE e = p ^ (prevE + fixedKey + offset);

		*ptr = e;

		prevP = p;
		prevE = e;
	}
}

void PacketBuffer::Decode(PacketBuffer* buffer)
{
	BYTE fixedKey = Fixed_Code;
	BYTE randomKey = *(buffer->GetBufferPtr() + 3);
	int dataSize = buffer->GetDataSize();
	char* start = buffer->GetBufferPtr();
	char* ptr = start;
	char* end = start + dataSize;
	ptr += 4;

	BYTE prevE = 0;
	BYTE prevP = 0;
	int  offset = 1;

	for (; ptr < end; ++ptr, ++offset)
	{
		BYTE c = *ptr;
		BYTE p = c ^ (prevE + fixedKey + offset);
		BYTE d = p ^ (prevP + randomKey + offset);
		*ptr = d;
		prevE = c;
		prevP = p;
	}
}




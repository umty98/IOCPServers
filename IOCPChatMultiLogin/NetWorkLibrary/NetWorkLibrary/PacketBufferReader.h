#pragma once
//#include <Windows.h>
//#include "TLSObjectPool.h"

class PacketBufferReader
{
public:
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400
	};

private:
	PacketBufferReader();
	PacketBufferReader(int bufferSize);

	friend class TLSMemoryPool<PacketBufferReader>;
public:
	static TLSMemoryPool<PacketBufferReader> pool;

	static PacketBufferReader* Alloc()
	{
		PacketBufferReader* p = pool.allocate();
		p->Clear();
		//PacketBufferReader* p = new PacketBufferReader;
		return p;
	}

	void Release()
	{
		pool.free(this);
		//delete this;
	}

	virtual ~PacketBufferReader();

	void Clear();
	int GetBufferSize() const { return p_BufferSize; }
	int GetDataSize() const { return p_WritePos - p_ReadPos; }
	char* GetBufferPtr() { return p_Buffer; }
	char* GetWriteBufferPtr() { return p_Buffer + p_WritePos; }
	char* GetReadBufferPtr() { return p_Buffer + p_ReadPos; }
	int GetReadPos() const { return p_ReadPos; }
	int GetWritePos() const { return p_WritePos; }


	int MoveWritePos(int size);
	int MoveReadPos(int size);

	bool GetError() const { return p_Error; }
	bool ResizeBuffer(int newSize);

	// Data Read operator >>
	PacketBufferReader& operator>>(unsigned char& value);
	PacketBufferReader& operator>>(char& value);

	PacketBufferReader& operator>>(short& value);
	PacketBufferReader& operator>>(unsigned short& value);

	PacketBufferReader& operator>>(int& value);
	PacketBufferReader& operator>>(unsigned int& value);

	PacketBufferReader& operator>>(long& value);
	PacketBufferReader& operator>>(unsigned long& value);

	PacketBufferReader& operator>>(float& value);

	PacketBufferReader& operator>>(double& value);
	PacketBufferReader& operator>>(__int64& value);

	int DequeueData(char* dest, int size);
	int EnqueueData(const char* src, int size);
	int PeekData(char* dest, int size);

	static int copyRest(PacketBufferReader* oldBuf, PacketBufferReader* newBuf);

protected:
	char* p_Buffer;
	int p_BufferSize;
	int p_DataSize;
	int p_ReadPos;
	int p_WritePos;
	int resizeCnt;
	bool p_Error;
};
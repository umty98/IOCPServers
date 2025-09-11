#pragma once
//#include <Windows.h>
//#include "TLSObjectPool.h"

class PacketBuffer
{
public:
	enum en_PACKET
	{
		eBUFFER_DEFAULT = 1400
	};

private:
	PacketBuffer();
	PacketBuffer(int bufferSize);

	friend class TLSMemoryPool<PacketBuffer>;
public:
	friend class CLanServer;
	static TLSMemoryPool<PacketBuffer> pool;

	static PacketBuffer* Alloc()
	{
		PacketBuffer* p = pool.allocate();
		p->Clear();

		//PacketBuffer* p = new PacketBuffer;
		return p;
	}

	static PacketBuffer* Alloc(int headerSize)
	{
		PacketBuffer* p = pool.allocate();
		p->Clear();
		p->MoveWritePos(headerSize);
		return p;
	}



	//static void Free(PacketBuffer* p)
	//{
	//	pool.free(p);
	//}

	void AddRef()
	{
		InterlockedIncrement(&refCount);
		//refCount++;
	}

	void Release()
	{
		if (InterlockedDecrement(&refCount) == 0)
		{
			pool.free(this);
			//delete this;
		}
		//delete this;
	}

	virtual ~PacketBuffer();

	void Clear();
	int GetBufferSize() const { return p_BufferSize; }
	int GetDataSize() const { return p_WritePos - p_ReadPos;}
	char* GetBufferPtr() const { return p_Buffer; }
	char* GetReadPtr() const { return p_Buffer + p_ReadPos; }

	int MoveWritePos(int size);
	int MoveReadPos(int size);

	bool GetError() const { return p_Error; }
	bool ResizeBuffer(int newSize);

	bool GetBufEncoded() const { return isEncode; }
	void SetBufEncodedTrue() { isEncode = true; }

	// Data Write operator <<
	PacketBuffer& operator<<(unsigned char value);
	PacketBuffer& operator<<(char value);

	PacketBuffer& operator<<(short value);
	PacketBuffer& operator<<(unsigned short value);

	PacketBuffer& operator<<(int value);
	PacketBuffer& operator<<(unsigned int value);

	PacketBuffer& operator<<(long value);
	PacketBuffer& operator<<(unsigned long value);

	PacketBuffer& operator<<(float value);

	PacketBuffer& operator<<(double value);
	PacketBuffer& operator<<(unsigned __int64 value);
	PacketBuffer& operator<<(INT64 value);

	// Data Read operator >>
	PacketBuffer& operator>>(unsigned char& value);
	PacketBuffer& operator>>(char& value);

	PacketBuffer& operator>>(short& value);
	PacketBuffer& operator>>(unsigned short& value);

	PacketBuffer& operator>>(int& value);
	PacketBuffer& operator>>(unsigned int& value);

	PacketBuffer& operator>>(long& value);
	PacketBuffer& operator>>(unsigned long& value);

	PacketBuffer& operator>>(float& value);

	PacketBuffer& operator>>(double& value);
	PacketBuffer& operator>>(unsigned __int64& value);
	PacketBuffer& operator>>(INT64& value);

	int PeekData(char* dest, int size);
	int EnqueueData(const char* src, int size);
	int EnqueueHeader(const char* src, int size);
	int DequeueData(char* dest, int size);

	//일단 실험용
	int EnqueueId(uint64_t sessionId);

	//encode decode
	void SetCheckSum(PacketBuffer* buffer);

	int GetPayLoadCheckSum(PacketBuffer* buffer);

	void Encode(PacketBuffer* buffer);

	void Decode(PacketBuffer* buffer);

protected:
	char* p_Buffer;
	int p_BufferSize;
	int p_DataSize;
	int p_ReadPos;
	int p_WritePos;
	int resizeCnt;
	long refCount;
	bool p_Error;
	bool isEncode;
};
#pragma once
#include <Windows.h>

template<typename T>
class LockFreeStack
{
private:
    // 내부 중첩 구조체 Node 선언
    struct Node
    {
        T data;
        Node* next;
    };

    alignas(8) volatile LONG64 top;
    volatile unsigned int globalTag;

    static const unsigned int TAG_BITS = 17;
    static const unsigned int PTR_BITS = 64 - TAG_BITS;  // 47비트
    static const unsigned long long PTR_MASK = ((1ULL << PTR_BITS) - 1);
    static const unsigned int TAG_MASK = ((1U << TAG_BITS) - 1);

    inline LONG64 Pack(Node* ptr, unsigned int tag) const
    {
        return (((unsigned long long)tag & TAG_MASK) << PTR_BITS) | (((unsigned long long)ptr) & PTR_MASK);
    }

    inline void Unpack(LONG64 value, Node*& ptr, unsigned int& tag) const
    {
        tag = (unsigned int)((value >> PTR_BITS) & TAG_MASK);
        ptr = (Node*)(value & PTR_MASK);
    }

    static TLSMemoryPool<Node> pool;

public:
    LockFreeStack() : top(0), globalTag(0) {}
    ~LockFreeStack() {}

    void Push(const T& data)
    {
        Node* newNode = pool.allocate();
        newNode->data = data;
        unsigned int newTag = InterlockedIncrement((volatile LONG*)&globalTag);

        Node* oldPtr;
        unsigned int oldTag;
        LONG64 oldTop, newTop;
        do
        {
            oldTop = top;
            Unpack(oldTop, oldPtr, oldTag);
            newNode->next = oldPtr;
            newTop = Pack(newNode, newTag);
        } while (InterlockedCompareExchange64((volatile LONG64*)&top, newTop, oldTop) != oldTop);
    }

    bool Pop(T& outData)
    {
        Node* oldPtr;
        unsigned int oldTag;
        LONG64 oldTop, newTop;
        unsigned int newTag = InterlockedIncrement((volatile LONG*)&globalTag);
        do
        {
            oldTop = top;
            Unpack(oldTop, oldPtr, oldTag);
            newTop = Pack(oldPtr->next, newTag);
        } while (InterlockedCompareExchange64((volatile LONG64*)&top, newTop, oldTop) != oldTop);

        outData = oldPtr->data;
        pool.free(oldPtr);
        return true;
    }

    int Size()
    {
        int cnt = 0;
        Node* curr;
        unsigned int dummyTag;
        Unpack(top, curr, dummyTag);
        while (curr != nullptr)
        {
            curr = curr->next;
            cnt++;
        }
        return cnt;
    }

    void ShowPoolStatus()
    {
        pool.ShowInterface();
    }
};

#pragma once
#include <Windows.h>
#include <iostream>
#include <stdint.h>
#include "ObjectPool.h"
#include <vector>
#include "TLSObjectPool.h"

template<typename T>
class LockFreeQueue
{
private:
    struct Node
    {
        T data;
        Node* next;
        // Node() : next(nullptr) {}
    };

    // 실제 큐에 들어 있는 노드 수
    volatile long size;

    // 전역적으로 태그를 증가시키기 위한 변수
    volatile unsigned int globalTag;

    // 47비트 포인터 + 17비트 태그 (총 64비트)
    static const unsigned int TAG_BITS = 17;
    static const unsigned int PTR_BITS = 64 - TAG_BITS;  // 47비트
    static const uintptr_t PTR_MASK = ((1ULL << PTR_BITS) - 1);
    static const unsigned int TAG_MASK = ((1U << TAG_BITS) - 1);

    // head, tail에는 '태그가 섞인 포인터'를 저장
    // 하지만 Node::next에는 '순수 포인터'를 저장
    alignas(8)  Node* head;
    alignas(64) Node* tail;

    // 포인터와 태그를 하나의 uintptr_t로 합친 후, Node*로 캐스팅
    inline Node* Pack(Node* ptr, unsigned int tag) const
    {
        uintptr_t packed = (((uintptr_t)tag & TAG_MASK) << PTR_BITS) | (reinterpret_cast<uintptr_t>(ptr) & PTR_MASK);
        return reinterpret_cast<Node*>(packed);
    }

    // packed Node* 값을 받아서 원래의 포인터와 태그로 분리
    inline void Unpack(Node* packedPtr, Node*& realPtr, unsigned int& tag) const
    {
        uintptr_t v = reinterpret_cast<uintptr_t>(packedPtr);
        tag = static_cast<unsigned int>((v >> PTR_BITS) & TAG_MASK);
        realPtr = reinterpret_cast<Node*>(v & PTR_MASK);
    }

    //ObjectPool<Node> pool;
    static TLSMemoryPool<Node> pool;
    //TLSMemoryPool<Node> pool;
public:
    LockFreeQueue() : size(0), globalTag(1)
    {
        // 더미(dummy) 노드 생성
       // Node* dummy = new Node();
        Node* dummy = pool.allocate();
        //dummy->data = static_cast<T>(nullptr);
        //dummy->next = nullptr;
        dummy->next = (Node*)this;

        // head, tail에 Pack을 사용해 태그(0)와 함께 저장
        head = Pack(dummy, 0);
        tail = Pack(dummy, 0);
    }

    ~LockFreeQueue()
    {
        //일단 lockfreequeue에서 남아 있는거 반환안될가능성도 있네....
        Node* headPtr;
        unsigned int headTag;
        Unpack(head, headPtr, headTag);
        
        pool.free(headPtr);

        // 필요하면 남은 노드를 모두 Dequeue해서 해제하거나,
        // 안전하게 메모리를 해제하는 로직을 추가하세요.
    }

    void Enqueue(const T& data)
    {
        // Node* node = new Node();
        Node* node = pool.allocate();
        // node->next = nullptr;

        node->data = data;
        //node->next = nullptr;
        node->next = (Node*)this;

        while (true)
        {
            // tail에서 실제 포인터와 태그를 꺼낸다
            Node* tailPacked = tail;
            Node* tailPtr;
            unsigned int tailTag;
            Unpack(tailPacked, tailPtr, tailTag);

            Node* nextPtr = tailPtr->next; // next는 "순수 포인터"

            if (nextPtr == nullptr)
                continue;

            if (nextPtr == (Node*)this)
            {
                // tailPtr->next가 비어 있으면 새 노드 연결 시도
                // next에는 태그 없이 'node'를 그대로 저장
                if (InterlockedCompareExchangePointer(
                    reinterpret_cast<volatile PVOID*>(&tailPtr->next),
                    node,
                    (Node*)this) == (Node*)this)
                {
                    // 연결 성공 -> tail만 갱신 (패킹해서 저장)
                  //  Node* newTailPacked = Pack(node, tailTag + 1);
                    //unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));
                    unsigned int newTag = globalTag++;
                    Node* newTailPacked = Pack(node, newTag);
                    InterlockedCompareExchangePointer(
                        reinterpret_cast<volatile PVOID*>(&tail),
                        newTailPacked,
                        tailPacked);
                    InterlockedIncrement(&size);
                    return;
                }
            }
            else
            {
                // tail이 뒤처졌으면 tail만 앞으로 당김
               // Node* newTailPacked = Pack(nextPtr, tailTag + 1);
                //unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));
                unsigned int newTag = globalTag++;
                Node* newTailPacked = Pack(nextPtr, newTag);
                InterlockedCompareExchangePointer(
                    reinterpret_cast<volatile PVOID*>(&tail),
                    newTailPacked,
                    tailPacked);
            }
        }
    }

    int Dequeue(T& outData)
    {
        if (InterlockedDecrement(&size) < 0)
        {
            InterlockedIncrement(&size);
            return -1;
        }

        while (true)
        {
            Node* headPacked = head;
            Node* headPtr;
            unsigned int headTag;
            Unpack(headPacked, headPtr, headTag);
            Node* nextPtr = headPtr->next;

            Node* tailPacked = tail;
            Node* tailPtr;
            unsigned int tailTag;
            Unpack(tailPacked, tailPtr, tailTag);
            Node* nextTailPtr = tailPtr->next;

            if (nextTailPtr == nullptr || nextPtr == nullptr)
                continue;

            if (nextTailPtr != (Node*)this)
            {
                //unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));
                unsigned int newTag = globalTag++;
                Node* newTailPacked = Pack(nextTailPtr, newTag);
                InterlockedCompareExchangePointer(
                    reinterpret_cast<volatile PVOID*>(&tail),
                    newTailPacked,
                    tailPacked);

                continue;
            }

            if (nextPtr == (Node*)this)
            {
                // 실제로 비어 있음 -> size 보정 후 종료7
                InterlockedIncrement(&size);
                return -1;
            }

            else
            {
                // nextPtr는 이미 '진짜 포인터'이므로 그대로 접근 가능
                outData = nextPtr->data;

                // head를 nextPtr로 옮기며 태그 증가
               // Node* newHeadPacked = Pack(nextPtr, headTag + 1);
                //unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));
                unsigned int newTag = globalTag++;

                Node* newHeadPacked = Pack(nextPtr, newTag);

                if (InterlockedCompareExchangePointer(
                    reinterpret_cast<volatile PVOID*>(&head),
                    newHeadPacked,
                    headPacked) == headPacked)
                {
                    // head 업데이트 성공 시, 이전 dummy 노드를 해제
                    // delete headPtr;
                    pool.free(headPtr);
                    return 0;
                }
            }
        }
    }

    void PeekData(std::vector<PacketBuffer*>& outVector, int bufCount) const
    {
        int size = bufCount;
        Node* headPtr;
        unsigned int headTag;
        Unpack(head, headPtr, headTag);

        Node* current = headPtr->next;
        for (int i = 0; i < size; i++)
        {
            outVector.emplace_back(current->data);
            current = current->next;
        }
    }

    void PeekData(PacketBuffer* outArray[], int bufCount) const
    {
        int size = bufCount;
        Node* headPtr;
        unsigned int headTag;
        Unpack(head, headPtr, headTag);

        Node* current = headPtr->next;
        for (int i = 0; i < size; i++)
        {
            outArray[i] = current->data;
            current = current->next;
        }
    }

    int Size() const 
    {
        return static_cast<int>(size);
    }

    // 큐 내용을 모두 출력 (디버깅용)
    void PrintAllDataInQueue()
    {
        // head에서 실제 포인터를 꺼냄
        Node* headPtr;
        unsigned int headTag;
        Unpack(head, headPtr, headTag);
        int cnt = 0;
        // 더미 노드를 건너뛰고 실제 데이터부터 순회
       // Node* current = headPtr->next;
        Node* current = headPtr;
        while (current != (Node*)this)
        {
            std::cout << current->data << " ";
            current = current->next;
            cnt++;
        }
        std::cout << std::endl;
        std::cout << "남은 큐 개수 : " << cnt << '\n';
    }

    void PrintHeadTail()
    {
        // head 언패킹
        Node* headPtr;
        unsigned int headTag;
        Unpack(head, headPtr, headTag);

        // tail 언패킹
        Node* tailPtr;
        unsigned int tailTag;
        Unpack(tail, tailPtr, tailTag);

        std::cout << "[HeadTag=" << headTag << "] ";
        if (headPtr) std::cout << "Head data: " << headPtr->data << std::endl;

        std::cout << "[TailTag=" << tailTag << "] ";
        if (tailPtr) std::cout << "Tail data: " << tailPtr->data << std::endl;
    }

    void ShowPoolStatus()
    {
        pool.ShowInterface();
    }
};

template<typename T>
TLSMemoryPool<typename LockFreeQueue<T>::Node> LockFreeQueue<T>::pool;
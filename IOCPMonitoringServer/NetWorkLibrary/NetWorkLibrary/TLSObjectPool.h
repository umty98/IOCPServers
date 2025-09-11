#pragma once
#include <Windows.h>
#include <cstdio>
#include <new>

template<typename U>
struct o_Node
{
    U data;
    o_Node<U>* next;
};

template<typename U>
struct Bucket
{
    o_Node<U>* bucketTop;
    Bucket<U>* next;
};

template<typename U>
class BucketAllocPool
{
private:
    volatile LONG64 freelist;

    volatile long usedCnt;
    volatile long freeCnt;
    volatile long allocCnt;

    // ABA 방지용 태그 증가 변수
    volatile unsigned int globalTag;

    // 태그와 포인터를 나눠 쓰기 위한 비트 정의
    static const unsigned int TAG_BITS = 17;               // 상위 17비트를 태그로
    static const unsigned int PTR_BITS = 64 - TAG_BITS;    // 하위 47비트를 포인터로
    static const unsigned long long PTR_MASK = ((1ULL << PTR_BITS) - 1);
    static const unsigned int TAG_MASK = ((1U << TAG_BITS) - 1);

    // (포인터, 태그) → 64비트 값
    inline LONG64 Pack(Bucket<U>* ptr, unsigned int tag) const
    {
        return ((static_cast<unsigned long long>(tag) & TAG_MASK) << PTR_BITS) |
            (reinterpret_cast<unsigned long long>(ptr) & PTR_MASK);
    }

    // 64비트 값 → (포인터, 태그)
    inline void Unpack(LONG64 val, Bucket<U>*& ptr, unsigned int& tag) const
    {
        tag = static_cast<unsigned int>((val >> PTR_BITS) & TAG_MASK);
        ptr = reinterpret_cast<Bucket<U>*>(val & PTR_MASK);
    }

    volatile long totalAllocCnt;

public:
    BucketAllocPool() : freelist(0), usedCnt(0), freeCnt(0), allocCnt(0), globalTag(0), totalAllocCnt(0)
    {
    }

    ~BucketAllocPool()
    {
       // printf("BucketAllocPool : %d\n", totalAllocCnt);
        // freelist에 남은 모든 노드를 해제
        int cnt = 0;
        Bucket<U>* current;
        unsigned int dummyTag;
        LONG64 curVal = freelist;
        Unpack(curVal, current, dummyTag);
        while (current)
        {
            Bucket<U>* temp = current;
            current = current->next;
            cnt++;
            ::free(temp);
        }
    }

    // U타입의 메모리 블록 할당
    Bucket<U>* allocate()
    {
        Bucket<U>* oldPtr;
        unsigned int oldTag;
        LONG64 oldVal, newVal;

        // 새 태그를 전역적으로 증가
        unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));

        while (true)
        {
            // freelist의 현재 값(포인터+태그)을 원자적으로 읽어온다
            oldVal = freelist;
            Unpack(oldVal, oldPtr, oldTag);

            // freelist가 비어 있으면 새로 malloc
            if (!oldPtr)
            {
                return nullptr;
            }

            // pop: oldPtr->next를 head로 설정
            Bucket<U>* nextPtr = oldPtr->next;
            newVal = Pack(nextPtr, newTag);

            // CAS로 freelist를 갱신
            if (InterlockedCompareExchange64(&freelist, newVal, oldVal) == oldVal)
            {
                // pop 성공
                //InterlockedDecrement(&freeCnt);
                //InterlockedIncrement(&usedCnt);
                InterlockedIncrement(&totalAllocCnt);
                return oldPtr;
            }
            // 실패 시 다른 스레드가 freelist를 바꿨으므로 재시도
        }
    }

    void free(Bucket<U>* object)
    {
       //Bucket<U>* node = reinterpret_cast<Bucket<U>*>(object);
        Bucket<U>* node = object;
        Bucket<U>* oldPtr;
        unsigned int oldTag;
        LONG64 oldVal, newVal;

        unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));

        while (true)
        {
            oldVal = freelist;
            Unpack(oldVal, oldPtr, oldTag);

            // push: node->next = oldPtr
            node->next = oldPtr;
            newVal = Pack(node, newTag);

            // CAS로 freelist를 node로 갱신
            if (InterlockedCompareExchange64(&freelist, newVal, oldVal) == oldVal)
            {
                //InterlockedDecrement(&usedCnt);
               // InterlockedIncrement(&freeCnt);
                return;
            }
        }
    }
};

template<typename U>
class BucketRemainPool
{
private:
    // freelist: 상위 비트는 태그, 하위 비트는 o_Node* 포인터
    volatile LONG64 freelist;

    // 통계 변수 (원자적 접근)
    volatile long usedCnt;
    volatile long freeCnt;
    volatile long allocCnt;

    // ABA 방지용 태그 증가 변수
    volatile unsigned int globalTag;

    // 태그와 포인터를 나눠 쓰기 위한 비트 정의
    static const unsigned int TAG_BITS = 17;               // 상위 17비트를 태그로
    static const unsigned int PTR_BITS = 64 - TAG_BITS;    // 하위 47비트를 포인터로
    static const unsigned long long PTR_MASK = ((1ULL << PTR_BITS) - 1);
    static const unsigned int TAG_MASK = ((1U << TAG_BITS) - 1);

    // (포인터, 태그) → 64비트 값
    inline LONG64 Pack(Bucket<U>* ptr, unsigned int tag) const
    {
        return ((static_cast<unsigned long long>(tag) & TAG_MASK) << PTR_BITS) |
            (reinterpret_cast<unsigned long long>(ptr) & PTR_MASK);
    }

    // 64비트 값 → (포인터, 태그)
    inline void Unpack(LONG64 val, Bucket<U>*& ptr, unsigned int& tag) const
    {
        tag = static_cast<unsigned int>((val >> PTR_BITS) & TAG_MASK);
        ptr = reinterpret_cast<Bucket<U>*>(val & PTR_MASK);
    }

public:
    BucketRemainPool() : freelist(0), usedCnt(0), freeCnt(0), allocCnt(0), globalTag(0)
    {
    }

    ~BucketRemainPool()
    {
      //  printf("BucketRemainPool\n");
       // printf("usedCnt: %ld / freeCnt: %ld / allocCnt: %ld\n", usedCnt, freeCnt, allocCnt);
        // freelist에 남은 모든 노드를 해제

        int cnt = 0;
        Bucket<U>* current;
        unsigned int dummyTag;
        LONG64 curVal = freelist;
        Unpack(curVal, current, dummyTag);
        while (current)
        {
            Bucket<U>* temp = current;
            current = current->next;
            cnt++;
            ::free(temp);
        }
    }

    // U타입의 메모리 블록 할당
    Bucket<U>* allocate()
    {
        Bucket<U>* oldPtr;
        unsigned int oldTag;
        LONG64 oldVal, newVal;

        // 새 태그를 전역적으로 증가
        unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));

        while (true)
        {
            // freelist의 현재 값(포인터+태그)을 원자적으로 읽어온다
            oldVal = freelist;
            Unpack(oldVal, oldPtr, oldTag);

            // freelist가 비어 있으면 새로 malloc
            if (!oldPtr)
            {
                Bucket<U>* bucket = new Bucket<U>;
                InterlockedIncrement(&allocCnt);
                InterlockedIncrement(&usedCnt);
                return bucket;
            }
            // pop: oldPtr->next를 head로 설정
            Bucket<U>* nextPtr = oldPtr->next;
            newVal = Pack(nextPtr, newTag);

            // CAS로 freelist를 갱신
            if (InterlockedCompareExchange64(&freelist, newVal, oldVal) == oldVal)
            {
                // pop 성공
                InterlockedDecrement(&freeCnt);
                InterlockedIncrement(&usedCnt);
                return oldPtr;
            }
            // 실패 시 다른 스레드가 freelist를 바꿨으므로 재시도
        }
    }

    // U타입의 메모리 블록 반환
    void free(Bucket<U>* object)
    {
        //Bucket<U>* node = reinterpret_cast<Bucket<void>*>(object);
        Bucket<U>* node = object;

        Bucket<U>* oldPtr;
        unsigned int oldTag;
        LONG64 oldVal, newVal;

        // 새 태그를 전역적으로 증가
        unsigned int newTag = InterlockedIncrement(reinterpret_cast<volatile LONG*>(&globalTag));

        while (true)
        {
            oldVal = freelist;
            Unpack(oldVal, oldPtr, oldTag);

            // push: node->next = oldPtr
            node->next = oldPtr;
            newVal = Pack(node, newTag);

            // CAS로 freelist를 node로 갱신
            if (InterlockedCompareExchange64(&freelist, newVal, oldVal) == oldVal)
            {
                InterlockedDecrement(&usedCnt);
                InterlockedIncrement(&freeCnt);
                return;
            }
            // 실패 시 재시도
        }
    }
};

template<typename U>
class TLSMemoryPool {
private:
    static BucketAllocPool<U> globalAllocPool;
    static BucketRemainPool<U> globalRemainPool;

    struct LocalPool
    {
        o_Node<U>* freeListHead;
        size_t freeCount;

        o_Node<U>* remainListHead;
        size_t remainCount;

        LocalPool() : freeListHead(nullptr), freeCount(0),
            remainListHead(nullptr), remainCount(0)
        {
            //printf("local pool 생성\n");
        }
        ~LocalPool()
        {
            //printf("local pool 삭제\n");
            while (freeListHead)
            {
                o_Node<U>* temp = freeListHead;
                freeListHead = freeListHead->next;
                temp->data.~U();
                ::free(temp);
            }
            // remainList의 노드들도 필요하다면 동일하게 처리
            while (remainListHead)
            {
                o_Node<U>* temp = remainListHead;
                remainListHead = remainListHead->next;
                temp->data.~U();
                ::free(temp);
            }
        }
    };

    static thread_local LocalPool localPool;

    static const size_t FREE_LIST_THRESHOLD = 2000;
    static const size_t REMAIN_BALANCE_THRESHOLD = 2000;

public:
    TLSMemoryPool()
    {
        printf("tlsMemoryPool 생성자호출\n");
    }
    ~TLSMemoryPool()
    {
        printf("tlsMemoryPool 소멸자호출\n");   
    }

    U* allocate()
    {
        o_Node<U>* newNode;

        if (localPool.remainListHead)
        {
            newNode = localPool.remainListHead;
            localPool.remainListHead = newNode->next;
            localPool.remainCount--;
            return reinterpret_cast<U*>(newNode);
        }

        if (localPool.freeListHead)
        {
            newNode = localPool.freeListHead;
            localPool.freeListHead = newNode->next;
            localPool.freeCount--;
            return reinterpret_cast<U*>(newNode);
        }


        Bucket<U>* bucket = globalAllocPool.allocate();
        if (bucket == nullptr)
        {
            newNode = reinterpret_cast<o_Node<U>*>(malloc(sizeof(o_Node<U>)));
           // printf("malloc성공\n");
            if (newNode != nullptr)
                new (&newNode->data) U();

            return reinterpret_cast<U*>(newNode);
        }
        else
        {
           // newNode = bucket->bucketTop;
            newNode = reinterpret_cast<o_Node<U>*>(bucket->bucketTop);
            localPool.freeListHead = newNode->next;
            localPool.freeCount = FREE_LIST_THRESHOLD;
            localPool.freeCount--;
            globalRemainPool.free(bucket);

            return reinterpret_cast<U*>(newNode);
        }
    }

    void free(U* object)
    {
        o_Node<U>* node = reinterpret_cast<o_Node<U>*>(object);

        if (localPool.freeCount < FREE_LIST_THRESHOLD)
        {
            node->next = localPool.freeListHead;
            localPool.freeListHead = node;
            localPool.freeCount++;
            //printf("thread id : %d / freeCnt : %d\n", GetCurrentThreadId(), localPool.freeCount);
        }
        else
        {
            // printf("remain input\n");
            node->next = localPool.remainListHead;
            localPool.remainListHead = node;
            localPool.remainCount++;

            if (localPool.remainCount >= REMAIN_BALANCE_THRESHOLD)
            {
                //printf("bucketrebalance\n");
                packRemainListIntoBucket();
            }
        }
    }

private:
    void packRemainListIntoBucket()
    {
        localPool.remainCount = 0;
        Bucket<U>* bucket = globalRemainPool.allocate();
       // bucket->bucketTop = localPool.remainListHead;
        //bucket->bucketTop = reinterpret_cast<o_Node<void>*>(localPool.remainListHead);
        //localPool.remainListHead = 0;
        bucket->bucketTop = localPool.freeListHead;
        localPool.freeListHead = localPool.remainListHead;
        localPool.remainListHead = 0;
        bucket->next = nullptr;
        globalAllocPool.free(bucket);
    }
};

template<typename U>
BucketAllocPool<U> TLSMemoryPool<U>::globalAllocPool;

template<typename U>
BucketRemainPool<U> TLSMemoryPool<U>::globalRemainPool;

template<typename U>
thread_local typename TLSMemoryPool<U>::LocalPool TLSMemoryPool<U>::localPool;
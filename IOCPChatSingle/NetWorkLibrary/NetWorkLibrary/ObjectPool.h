#pragma once
#include <Windows.h>
#include <cstdio>

// ObjectPool은 U타입의 raw memory 블록을 관리합니다.
template<typename U>
class ObjectPool
{
private:
    // 내부 노드는 다음 포인터만 보유합니다. (데이터 자체는 U에 대응)
    struct o_Node
    {
        o_Node* next;
    };

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
    inline LONG64 Pack(o_Node* ptr, unsigned int tag) const
    {
        return ((static_cast<unsigned long long>(tag) & TAG_MASK) << PTR_BITS) |
            (reinterpret_cast<unsigned long long>(ptr) & PTR_MASK);
    }

    // 64비트 값 → (포인터, 태그)
    inline void Unpack(LONG64 val, o_Node*& ptr, unsigned int& tag) const
    {
        tag = static_cast<unsigned int>((val >> PTR_BITS) & TAG_MASK);
        ptr = reinterpret_cast<o_Node*>(val & PTR_MASK);
    }

public:
    ObjectPool() : freelist(0), usedCnt(0), freeCnt(0), allocCnt(0), globalTag(0)
    {
    }

    ~ObjectPool()
    {
       // printf("ObjectPool 소멸자 호출\n");

        // freelist에 남은 모든 노드를 해제
        int cnt = 0;
        o_Node* current;
        unsigned int dummyTag;
        LONG64 curVal = freelist;
        Unpack(curVal, current, dummyTag);
        while (current)
        {
            o_Node* temp = current;
            current = current->next;
            cnt++;
            ::free(temp);
        }
       // printf("해제 cnt : %d\n", cnt);
    }

    // U타입의 메모리 블록 할당
    U* allocate()
    {
        o_Node* oldPtr;
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
                o_Node* node = reinterpret_cast<o_Node*>(malloc(sizeof(U)));
                if (!node) return nullptr;
                //InterlockedIncrement(&allocCnt);
                //InterlockedIncrement(&usedCnt);
                return reinterpret_cast<U*>(node);
            }

            // pop: oldPtr->next를 head로 설정
            o_Node* nextPtr = oldPtr->next;
            newVal = Pack(nextPtr, newTag);

            // CAS로 freelist를 갱신
            if (InterlockedCompareExchange64(&freelist, newVal, oldVal) == oldVal)
            {
                // pop 성공
                //InterlockedDecrement(&freeCnt);
                //InterlockedIncrement(&usedCnt);
                return reinterpret_cast<U*>(oldPtr);
            }
            // 실패 시 다른 스레드가 freelist를 바꿨으므로 재시도
        }
    }

    // U타입의 메모리 블록 반환
    void free(U* object)
    {
        o_Node* node = reinterpret_cast<o_Node*>(object);

        o_Node* oldPtr;
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
                //InterlockedDecrement(&usedCnt);
               // InterlockedIncrement(&freeCnt);
                return;
            }
            // 실패 시 재시도
        }
    }

    void ShowInterface()
    {
        long u = InterlockedCompareExchange(&usedCnt, 0, 0);
        long f = InterlockedCompareExchange(&freeCnt, 0, 0);
        long a = InterlockedCompareExchange(&allocCnt, 0, 0);
        printf("usedCnt: %ld / freeCnt: %ld / allocCnt: %ld\n", u, f, a);
    }
};



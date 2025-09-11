//#pragma once
//#include <Windows.h>
//
//// Send 상태를 64비트 값으로 표현  
//// 하위 1비트: send 플래그 (1이면 전송 중, 0이면 전송 중 아님)
//// 상위 63비트: sendQueue 항목 개수
//#define SEND_FLAG_MASK      0x1LL
//#define SEND_SIZE_SHIFT     1
//#define GET_QUEUE_SIZE(x)   ((x) >> SEND_SIZE_SHIFT)
//#define MAKE_SEND_STATE(size, flag) (((size) << SEND_SIZE_SHIFT) | ((flag) ? 1LL : 0LL))
//
//// 전송 요청 전에 플래그를 세우려고 시도: sendQueue에 항목이 있고 아직 전송 중이 아니면 플래그를 1로 세팅한다.
//inline bool TrySetSendFlag(volatile LONG64* sendState)
//{
//    LONG64 oldState, newState;
//    do {
//        oldState = *sendState;   // 현재 상태 읽기
//        if (oldState & SEND_FLAG_MASK)  // 이미 전송 중이면 실패
//            return false;
//        if (GET_QUEUE_SIZE(oldState) == 0)  // 보낼 항목이 없다면 실패
//            return false;
//        newState = oldState | SEND_FLAG_MASK;  // 플래그만 1로 설정 (큐 개수는 그대로)
//    } while (InterlockedCompareExchange64(sendState, newState, oldState) != oldState);
//    return true;
//}
//
//// Enqueue 시 호출: sendQueue에 항목 추가(항목 수 +1); 플래그는 그대로 유지
//inline void AtomicEnqueue(volatile LONG64* sendState)
//{
//    LONG64 oldState, newState;
//    do {
//        oldState = *sendState;
//        newState = MAKE_SEND_STATE(GET_QUEUE_SIZE(oldState) + 1, (oldState & SEND_FLAG_MASK) != 0);
//    } while (InterlockedCompareExchange64(sendState, newState, oldState) != oldState);
//}
//
//// Send 완료 후, 항목 하나 제거: 큐 사이즈를 -1; 만약 항목이 모두 제거되면 플래그도 0으로 해제
//inline void AtomicDequeue(volatile LONG64* sendState)
//{
//    LONG64 oldState, newState;
//    do {
//        oldState = *sendState;
//        newState = MAKE_SEND_STATE(GET_QUEUE_SIZE(oldState) - 1, (oldState & SEND_FLAG_MASK) != 0);
//    } while (InterlockedCompareExchange64(sendState, newState, oldState) != oldState);
//}
//
//inline void AtomicResetSendFlag(volatile LONG64* sendState)
//{
//    LONG64 oldState, newState;
//    do {
//        oldState = *sendState;
//
//        newState = MAKE_SEND_STATE(GET_QUEUE_SIZE(oldState), false);
//    } while (InterlockedCompareExchange64(sendState, newState, oldState) != oldState);
//}
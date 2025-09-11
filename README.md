IOCPChatSingle = Single Content Thread를 기반으로 일감을 직렬 처리하는 서버
IOCPChatMultiLogin = IOCP Worker Thread에서 공유 자원에 락을 걸고 일감을 병렬 처리하는 서버(Redis를 활용한 토큰으로 로그인 인증 과정 포함)
IOCPLoginServer = 퍼블리셔에서 가져온 토큰을 바탕으로 DB에서 토큰 조회후 Chat,GameServer에서 로그인 인증에 필요한 토큰 Redis에 저장
IOCPMonitoringServer = 각 서버들로 부터 수집한 데이터를 일정 시간 마다 DB에 저장(CCTV역할) 모니터링 클라이언트는 모니터링 서버에 접속하여 받은 데이터를 그래프로 보여줌

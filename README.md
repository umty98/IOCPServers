IOCPChatSingle = Single Content Thread를 기반으로 JobQueue를 통해서 일감을 주고 받으면 일감에 대해서 직렬처리
IOCPChatMultiLogin = IOCP Worker Thread에서 직접 공유 자원에 락을 걸고 일감을 병렬 처리 Redis의 기능을 사용하여 토큰을 로그인 서버와 주고 받으며 로그인 인증 과정 포함
IOCPLoginServer = 로그인을 담당하는 서버로 분산 설계를 하였음. ChatServer에서 인증할 토큰을 DB에 MySQL을 사용하여 확인하고 Redis에 저장하는 역할
IOCPMonitoringServer = 각 서버로 부터 수신받은 데이터를 일정 시간마다 모니터링 서버에 일괄적으로 저장 CCTV 역할을 함, 모니터링 클라가 외부에서 접속하여 데이터를 그래프로 보일 수 있도록 데이터 전송도함

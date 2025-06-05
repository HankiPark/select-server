# 🌐 Select 기반 MMO 서버

단일 스레드 기반으로 설계된 C++ MMO 서버입니다.  

select() 시스템 콜을 활용하여 최대 7,000명의 LAN 클라이언트 동시 접속을 처리하며, 

커스텀 프로토콜, 이벤트 기반 처리 구조, 메모리 풀을 직접 구현했습니다.

---

## ⚙️ 기술 스택

- **Language**: C++
- **Platform**: Windows (Visual Studio 2022)
- **Network**: Winsock2, select()
- **Memory**: 메모리 풀
- **Concurrency**: 단일 스레드 이벤트 루프
- **Protocol**: 바이너리 기반 커스텀 프로토콜
- **Testing Tools**: GCP VM, Wireshark, 테스트 클라이언트

---

## 📐 서버 구조

<pre>
[NetworkProcess() → PacketProc() → UpdateLogic()]
</pre>

- select 기반 단일 루프에서 수신, 파싱, 전송까지 모두 처리
- 각 소켓 FD에 대한 상태 정보는 세션 구조체에 저장

---

## 🧩 주요 기능

- ✅ 최대 7,000명 LAN 환경 동시 처리
- ✅ 커스텀 프로토콜: 길이 기반 파싱 + 명령 ID 처리
- ✅ memory pool + allocator 직접 구현 및 적용
- ✅ sendQueue 기반 전송 → write-ready 시 전송
- ✅ AOI 기반 유저 동기화 로직 적용
- ✅ FD_SET 최적화를 통한 성능 유지

---

## 📊 성능 테스트

| 테스트 항목 | 수치 |
|------------|------|
| 최대 동시 접속자 | 7000 clients (LAN) |
| 초당 전송 처리량 (패킷 평균 11byte) | 2.19MB/s |
| Sync 패킷 오류 | 17시간 30분 테스트 중 1회 |
| CPU 사용률 | 평균 17% |

---

## 🧪 문제 & 개선 포인트

- 일정 인원 이상 접속 시 recv, send에 문제 발생
  → 루프마다 fd_set의 초기화가 누락되어 발생한 문제, FD_ZERO를 매 루프마다 추가하여 해결
- 원인을 알 수 없는 성능 모니터의 retransmitted의 높은 수치
  → 지속적인 send 호출이 가장 큰 문제라고 판단하여 루프 내에서 send 처리량이 일정 수준에 도달하면, Sleep을 활용해 네트워크의 부하를 완화

---

## 🖥️ 실행 방법

```bash
# 빌드
Open in Visual Studio 2022 / C++14 / Windows 10

# 실행
1. 'TCPFighter7000.sln' 열기
2. 빌드 -> 실행 (F5)
3. 클라이언트는 없음.
```

---

## 📦 커스텀 패킷 구조

| 바이트 위치 | 필드 이름   | 크기(Byte) | 설명                       |
|-------------|-------------|------------|----------------------------|
| 0           | `length`    | 1          | 전체 패킷 길이             |
| 1           | `type`      | 1          | 패킷 종류 (ENUM)          |
| 2           | `checksum`  | 1          | 단순 오류 검출용   |
| 3~n         | `payload`   | N          | 실제 데이터 (ex. 이동 좌표 등) |

모든 패킷은 3byte header와 가변 길이 payload로 구성됩니다.

payload는 각 패킷의 type에 따라 구조체가 다르게 해석되며, 이 구조는 CPacket class를 통해 파싱됩니다.

---

## 🧠 회고 및 학습 포인트
- select 코드 부근에서 while문을 사용한 지저분한 코드가 존재했는데, 이를 do - while로 개선하면서 코드의 유지보수성과 가독성이 매우 중요함을 느꼈습니다.
- select 기반 구조는 구현이 단순하지만, 대규모 연결을 처리하는데 있어, 단일 스레드의 한계가 느껴졌습니다.
- 실제로 인원이 증가했을 때, send로 인한 block으로 전체 루프가 지연되는 구조적 한계가 드러났습니다.

---

## 🔗 참고
- 포트폴리오 Notion Page : https://www.notion.so/2021475751608044aba0c9561216bc11?source=copy_link

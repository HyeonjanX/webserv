# Webserv (hyeonjanX)

https://github.com/HyeonjanX/webserv/assets/63810422/6a5e6cd9-3ee0-4b9d-afc0-f1f7ce8c35f4

HTTP/1.1 in action

## 구현 사항

* HTTP/1.1 서버 : C++98 표준을 준수하여 구현
* 다중 포트 리스닝 : 설정 파일을 통해 여러 포트에서 요청 처리
* HTTP 메소드 구현 : GET, POST, DELETE, PATCH
* 파일 업로드 구현 : 대용량 파일 업로드 지원
* Multipart/form-data 지원
* 정적 웹사이트 서비스
* CGI 스크립트 실행
* 비동기 I/O 처리 : `kqueue()`를 사용하여 비동기적으로 I/O 작업 처리
* 설정 파일을 통한 서버 설정 : JSON 형식의 파일로 서버 설정
* 스트레스 테스트 : `seige`를 통한 서버의 안정성 및 성능 평가

## 학습 요소

* 네트워크 프로그래밍 : 소켓 프로그래밍, I/O Multiplexing
* HTTP/1.1 프로토콜 이해 : HTTP 요청/응답 처리, 상태 코드, 헤더, 메서드
* CGI 인터페이스 : 웹 서버와 프로그램 간 데이터 교환 처리
* 서버 구성 및 관리 : Nginx의 설정 파일을 바탕으로 설정 파일 구성 및 해석

## 실행 방법

1. Clone Repository

```sh
git clone <repository_url>
```

2. Build Project

```sh
make
```

3. Run server with configuration file

```sh
./webserv configuration_file.json
```
* 만약 설정 파일 없이 실행하는 경우, 서버 기본 설정을 적용하여 실행
* 예제 파일은 `data/json`에서 확인 가능

#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <iostream>
#include <string>

void print_kevent_info(const struct kevent &ke)
{
    std::cout << "kevent information:" << std::endl;
    std::cout << "  ident:  " << (unsigned long)ke.ident << std::endl;
    std::cout << "  filter: " << ke.filter << std::endl;
    std::cout << "  flags:  " << ke.flags << std::endl;
    std::cout << "  fflags: " << ke.fflags << std::endl;
    std::cout << "  data:   " << (long)ke.data << std::endl;
    std::cout << "  udata:  " << ke.udata << std::endl;
}

/**
 * 로컬 파일을 읽어들일때, kqueue와 kevent.data를 사용해보는 코드
 * - 파일 오픈에는 open을 쓴다.
 * - EVFILT_READ 이벤트 발생시, kevent.data가 남은 파일의 바이트 수를 나타낸다.
 * - 당연히 파일에서 읽어야할 것이 남으면, EVFILT_READ는 한 번 더 발생한다.
 * - read() == kevent.data로 파일을 다 읽었음을 판별한다.
*/

void test(const char* filename)
{
     int fd = open(filename, O_RDONLY | O_NONBLOCK); // 파일을 non-blocking으로 열기
    if (fd == -1)
    {
        perror("open");
        std::exit(1);
    }

    int kq = kqueue();
    if (kq == -1)
    {
        perror("kqueue");
        std::exit(1);
    }

    struct kevent kev;
    EV_SET(&kev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL); // 읽기 이벤트 설정

    if (kevent(kq, &kev, 1, NULL, 0, NULL) == -1)
    { // 이벤트 등록
        perror("kevent");
        std::exit(1);
    }

    while (true)
    {
        struct kevent kev_received;
        if (kevent(kq, NULL, 0, &kev_received, 1, NULL) == -1)
        { // 이벤트 대기
            perror("kevent");
            std::exit(1);
        }

        print_kevent_info(kev_received);

        if (kev_received.flags & EV_EOF)
        {
            std::printf("EOF detected!\n");
        }
        else
        {
            char buffer[1024];
            ssize_t n = read(fd, buffer, sizeof(buffer)); // 실제 읽기 연산
            std::string data;
            data.append(buffer, n);
            if (n > 0)
            {
                // 처리 로직
                std::cout << "-*-*-*-*-*-*-*-*-*- Read " << n << " bytes -*-*-*-*-*-*-*-*-*-" << std::endl;
                std::cout << data << std::endl;
                std::cout << "-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*" << std::endl;

                // 끝
                if (n == kev_received.data)
                {
                    std::cout << "파일 읽기 완료" << std::endl;
                    break;
                }
            }
        }
    }

    close(fd);
    close(kq);
}

int main()
{
    test("lorem_ipsum.txt");
    test("123.txt");
    return 0;
}

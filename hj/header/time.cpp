#include <iostream>
#include <ctime>

void print_current_time() {
    std::string data;

    // 현재 시간을 time_t 형태로 가져옴
    std::time_t current_time = std::time(0);

    // tm 형태로 변환
    std::tm* time_info = std::localtime(&current_time);

    // 문자열 형태로 변환
    char buffer[80];
    size_t n = std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);

    // 출력
    std::cout << "Current time: " << buffer << std::endl;

    data.append("Date: ");
    data.append(buffer, n);

    std::cout << "|" << data << "|" << std::endl;
}

int main() {
    print_current_time();
    return 0;
}
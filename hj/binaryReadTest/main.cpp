#include <string>
#include <iostream>

int main(void)
{
    std::string data;
    char buf[] = { 'A', 'B', 'C', '\0', 'D', 'E' };
    data.append(buf, 6);

    // data의 실제 길이는 6이어야 하지만, .length()는 6을 반환하지 않을 수 있습니다.
    std::cout << "data.length(): " << data.length() << std::endl;
    std::cout << "data.size(): " << data.size() << std::endl;
    std::cout << "data: " << data << std::endl;

    // data.length(): 6
    // data.size(): 6
    // data: ABCDE
    // C++의 std::string은 사실상 바이트 배열을 저장하며, null 바이트(\0)를 포함할수 있어서 문제가 없었다.... ^^

    // GPT의 헛소리에 주의하자!

    return 0;
}
#ifndef HEADER_HPP
#define HEADER_HPP

#include <string>

class Header {
public:
    Header();
    Header(const std::string& name, const std::string& value);
    std::string getName() const;
    std::string getValue() const;
    void setName(const std::string& name);
    void setValue(const std::string& value);

private:
    std::string name;
    std::string value;
};

#endif // HEADER_HPP

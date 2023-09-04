#include "Header.hpp"

Header::Header() {}

Header::Header(const std::string& name, const std::string& value) : name(name), value(value) {}

std::string Header::getName() const {
    return name;
}

std::string Header::getValue() const {
    return value;
}

void Header::setName(const std::string& name) {
    this->name = name;
}

void Header::setValue(const std::string& value) {
    this->value = value;
}

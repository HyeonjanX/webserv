#include "Cgi.hpp"
#include <iostream>
#include <map>

int main()
{
    Cgi cgi;
    
    cgi.exec("GET", "");

    cgi.exec("POST", "This is data.");
    cgi.exec("POST", "");
    
    return 0;
}

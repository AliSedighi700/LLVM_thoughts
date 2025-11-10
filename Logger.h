#ifndef Logger_H
#define Logger_H

#include <iostream>
#include <sstream>

class ErrorLogMessage : public std::enable_shared_from_this<char>
{
public:

    ~ErrorLogMessage()
    {
        std::ostringstream oss;
        std::cerr << "Fatal Error: " << oss.str().c_str();
        exit(EXIT_FAILURE);
    }

#define DIE ErrorLogMessage()

private:
};


#endif

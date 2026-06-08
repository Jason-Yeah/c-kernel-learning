
#ifndef LOGGER_H
#define LOGGER_H

#include <iostream>
#include <string>
#include <type_traits>

namespace sen_std {
    template <typename T, typename Enable = void>
    class logger
    {
        public:
            static void log(const T& val)
            {
                std::cout << "General Logger: " << val << std::endl;
            }
    };

    
}; 

#endif  // LOGGER_H

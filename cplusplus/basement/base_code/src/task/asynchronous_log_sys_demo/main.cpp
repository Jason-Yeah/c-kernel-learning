#include "logger.h"
#include <exception>

int main()
{
    try 
    {
        logger logr;
        logr.add_sink(std::make_unique<console_sink>());

        logr.add_sink(
            std::make_unique<rotating_file_sink>(
                "log",
                1024 * 1024,
                true,
                ".txt"
            )
        );

        logr.log(log_level::INFO, "START...");

        int id = 10;
        std::string action = "login";
        double duration = 1.5;
        std::string field = "nss";

        logr.log(log_level::INFO,
                 "User {} performed {} in {} seconds",
                 id,
                 action,
                 duration);

        logr.log(log_level::DEBUG, "Hello {}", field);

        logr.log(log_level::WARN, "Multiple placeholder: {}", 1, 2, 3);
    } 
    catch (const std::exception& e) 
    {
        std::cerr << "Error is " << e.what() << std::endl;
    }
    

    return 0;
}
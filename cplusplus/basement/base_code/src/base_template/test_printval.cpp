#include "template.h"

int main() {
    int x = 42;
    int* p = &x;

    sen_std::printval(3.14);        // General print
    sen_std::printval(std::string("hello"));  // String print
    sen_std::printval(p);           // int* print (specialization)

    return 0;
}

#include "item31_widget.h"

#include <iostream>

int main() {
    Widget widget{"dashboard"};
    std::cout << widget.summary() << '\n';
}

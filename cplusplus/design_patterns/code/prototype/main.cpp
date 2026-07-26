#include "resume.hpp"

int main()
{
    auto prototype = std::make_unique<ConcreteResume>(
        "Jason", 24, "lit 4 years, xdu 2 years");
    prototype->show();

    auto resume1 = prototype->clone();
    // std::shared_ptr<T>::get() => return ptr;(Resume*)
    // dynamic_cast<ConcreteResume *>(resume1.get())->setExp("BAT");
    resume1->setExp("BAT 1");
    resume1->show();

    auto resume2 = prototype->clone();
    resume2->show();

    return 0;
}
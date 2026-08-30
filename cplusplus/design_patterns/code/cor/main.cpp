#include "handler.hpp"

int main()
{
    auto leader = std::make_shared<TeamLeader>();
    auto manager = std::make_shared<Manager>();
    auto director = std::make_shared<Director>();
    auto boss = std::make_shared<Boss>();

    leader->SetNext(manager)->SetNext(director)->SetNext(boss);

    std::cout << "=== 请 1 天假 ===" << std::endl;
    leader->HandleRequest(1);

    std::cout << "\n=== 请 3 天假 ===" << std::endl;
    leader->HandleRequest(3);

    std::cout << "\n=== 请 6 天假 ===" << std::endl;
    leader->HandleRequest(6);

    std::cout << "\n=== 请 30 天假 ===" << std::endl;
    leader->HandleRequest(30);

    return 0;
}

#include "mediator.hpp"

int main()
{
    auto chatRoom = std::make_shared<ChatRoom>();

    // 创建用户（同事），都只认识聊天室
    auto zhang = std::make_shared<User>("张三", chatRoom.get());
    auto li = std::make_shared<User>("李四", chatRoom.get());
    auto wang = std::make_shared<User>("王五", chatRoom.get());

    chatRoom->AddColleague(zhang);
    chatRoom->AddColleague(li);
    chatRoom->AddColleague(wang);

    zhang->Send("what's up bro.");

    li->Send("shut up man.");

    return 0;
}

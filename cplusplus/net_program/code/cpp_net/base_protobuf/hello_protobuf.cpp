// hello_protobuf.cpp — 最简单 protobuf 示例
// 编译方法（两步）：
//   1. protoc --cpp_out=. chat.proto               # 生成chat.pb.h/cc
//   2. g++ -std=c++17 hello_protobuf.cpp chat.pb.cc -lprotobuf -o hello
#include "chat.pb.h"
#include <iostream>
#include <string>

int main()
{
    // 1. 创建消息
    chat::ChatMessage msg;
    msg.set_sender(1001);
    msg.set_receiver(2002);
    msg.set_content("Hello from protobuf!");

    std::cout << "sender: " << msg.sender() << "\n";
    std::cout << "receiver: " << msg.receiver() << "\n";
    std::cout << "content: " << msg.content() << "\n";
    std::cout << "size: " << msg.ByteSizeLong() << " bytes\n";

    // 2. 序列化
    std::string wire;
    msg.SerializeToString(&wire);

    // 3. 反序列化
    chat::ChatMessage parsed;
    if (parsed.ParseFromString(wire))
    {
        std::cout << "parse OK: " << parsed.content() << "\n";
    }

    google::protobuf::ShutdownProtobufLibrary();
    return 0;
}

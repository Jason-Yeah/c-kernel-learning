# build.sh — 不用 cmake，手动编译 protobuf 示例
# 确保已安装：
#   sudo apt install protobuf-compiler libprotobuf-dev

set -e  # 遇到错误就退出

echo "=== 1. 编译 .proto ==="
protoc --cpp_out=. chat.proto
echo "  → chat.pb.h / chat.pb.cc 已生成"

echo ""
echo "=== 2. 编译 hello_protobuf ==="
g++ -std=c++17 hello_protobuf.cpp chat.pb.cc -lprotobuf -o hello
echo "  → ./hello"

echo ""
echo "=== 3. 编译 echo_server ==="
g++ -std=c++17 echo_server.cpp chat.pb.cc -lprotobuf -lboost_system -lpthread -o echo_server
echo "  → ./echo_server"

echo ""
echo "=== 4. 编译 echo_client ==="
g++ -std=c++17 echo_client.cpp chat.pb.cc -lprotobuf -lboost_system -lpthread -o echo_client
echo "  → ./echo_client"

echo ""
echo "=== 全部编译成功 ==="
echo "运行: ./echo_server &  && ./echo_client"

#include <iostream>
#include <string>
#include <sstream>

using std::string;

int main()
{
    string s1;          // 默认初始化，空字符串
    string s2(s1);      // s2是s1副本使用是拷贝构造函数
    string s3 = s1;     // 和上面等价
    string s4 = "jason";// 是人都能看懂
    string s5(5, 'c');  // 把s5初始化为由`4`个连续的字符'c'组成的串
    string s6(s4, 0, 4); //从0开始到长度为4

    string line;
    // std::getline(std::cin, line);
    s1.append("fku");
    size_t t = s4.find(s1);

    if (t != std::string::npos)
    {
        std::cout << "找到 '" << s1 << "' 在位置: " << t << std::endl;
    }
    else
    {
        std::cout << "'" << s1 << "' 未找到。" << std::endl;
    }

    std::string text = "I like cats.";
    std::string from = "cats";
    std::string to = "dogs";

    size_t pos = text.find(from);
    if (pos != std::string::npos) 
    {
        text.replace(pos, from.length(), to);
        std::cout << "替换后: " << text << std::endl; // 输出: I like dogs.
    } else 
    {
        std::cout << "'" << from << "' 未找到。" << std::endl;
    }

    try
    {
        /* code */
        std::cout << text.at(0) << std::endl;
        std::cout << text.at(100) << std::endl;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    // sstream
    std::string data = "pi 192.168.10.102";
    std::stringstream ss(data);

    std::string res = ss.str();
    std::cout << res << std::endl;

    std::string user;
    std::string ip;

    ss >> user >> ip;
    std::cout << user << "@" << ip << std::endl; 

    int num = 100;
    double pi = 3.141;

    std::string str1 = std::to_string(num);
    std::string str2 = std::to_string(pi);

    std::cout << "str1: " << str1 << ", str2: " << str2 << std::endl;
    
    return 0;
}
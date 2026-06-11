#include "stream_demo.h"

int main()
{
    double PI = 3.1415926535;
    std::cout << "PI: " << PI << std::endl;
    std::cout << "PI: " << std::fixed << std::setprecision(3) << PI << std::endl;

    std::ofstream outf("data.txt");
    if (!outf.is_open())
    {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }

    outf << "Jason\n";
    outf << "Ye\n";

    outf.close();

    std::ifstream ifs("singleton.h");
    if (!ifs.is_open())
    {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }

    std::string line;
    while (std::getline(ifs, line))
        std::cout << line << std::endl;
    ifs.close();

    puts("==============");

    std::fstream fs("data.txt", std::ios::in | std::ios::out |
        std::ios::app);
    if (!fs.is_open())
    {
        std::cerr << "无法打开文件" << std::endl;
        return 1;
    }

    fs << "appending...\n";
    fs.seekg(0, std::ios::beg);
    
    line.clear();
    while (std::getline(fs, line))
        std::cout << line << std::endl;
    fs.close();

    int num = 1234;
    std::string str = std::to_string(num);
    std::cout << str << std::endl;

    std::stringstream ss;

    ss << num << " " << str << " " << PI;
    auto res = ss.str();
    std::cout << res << std::endl;

    ss.clear();
    ss.str("");

    return 0;
}
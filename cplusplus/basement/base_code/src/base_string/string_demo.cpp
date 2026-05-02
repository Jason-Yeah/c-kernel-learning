#include <iostream>
#include <string>
#include <sstream>
#include <algorithm>
#include <map>

int main()
{
    std::string text;

    std::ostringstream oss;
    std::string line;
    while (std::getline(std::cin, line))
    {
        oss << line << " ";
    }
    text = oss.str();

    std::stringstream ss(text);
    std::string word;
    std::map<std::string, int> word_count;
    size_t wcnt = 0;
    std::string longest_w;

    while (ss >> word)
    {
        word.erase(std::remove_if(word.begin(), word.end(), 
            [](char c) { return ispunct(c); }), word.end());

        std::transform(word.begin(), word.end(), word.begin(), ::tolower);
        
        if (!word.empty()) {
            word_count[word]++;
            wcnt++;
            if (word.length() > longest_w.length()) {
                longest_w = word;
            }
        }
    }

    std::cout << "\n统计结果:\n";
    std::cout << "总单词数: " << wcnt << std::endl;
    std::cout << "每个单词出现的次数:\n";
    for (const auto& pair : word_count) {
        std::cout << pair.first << ": " << pair.second << std::endl;
    }

    std::cout << "最长的单词: " << longest_w << std::endl;

    // 查找指定单词的出现次数
    std::string searchWord;
    std::cout << "\n请输入要查找的单词: ";
    std::cin >> searchWord;
    // 转为小写
    std::transform(searchWord.begin(), searchWord.end(), searchWord.begin(), ::tolower);
    auto it = word_count.find(searchWord);
    if (it != word_count.end()) {
        std::cout << "'" << searchWord << "' 出现了 " << it->second << " 次。" << std::endl;
    } else {
        std::cout << "'" << searchWord << "' 未在文本中找到。" << std::endl;
    }

    return 0;
}
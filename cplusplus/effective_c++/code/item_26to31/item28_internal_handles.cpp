#include <iostream>
#include <string>
#include <utility>
#include <vector>

class Playlist {
public:
    explicit Playlist(std::vector<std::string> titles) : titles_(std::move(titles)) {}

    const std::string& titleAt(std::size_t index) const { return titles_.at(index); }
    std::string titleCopyAt(std::size_t index) const { return titles_.at(index); }
    void add(std::string title) { titles_.push_back(std::move(title)); }

private:
    std::vector<std::string> titles_;
};

int main() {
    Playlist playlist{{"first"}};
    const std::string& borrowed = playlist.titleAt(0);
    const std::string independentCopy = playlist.titleCopyAt(0);
    std::cout << "before change: " << borrowed << ", " << independentCopy << '\n';

    playlist.add("second"); // 可能触发 vector 重新分配，使 borrowed 悬空
    // 不再读取 borrowed；它的有效性已不能依赖。
    std::cout << "safe copy: " << independentCopy << '\n';
}

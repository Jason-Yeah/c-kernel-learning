#include <iostream>
#include <vector>
#include <algorithm>

class process
{
    public:
        process() = default;
        process(int threshold) : _threshold(threshold) {}
        void execute(std::vector<int>& data)
        {
            for(auto d: data)
                std::cout << d << " ";
            std::cout << std::endl;
            auto new_end = std::remove_if(data.begin(), data.end(), 
                [this](int x) {
                    return x < _threshold;
                });
            data.erase(new_end, data.end());
            for(auto d: data)
                std::cout << d << " ";
            std::cout << std::endl;
        }
    private:
        int _threshold;
};

int main()
{
    std::vector<int> data = {1, 6, 3, 8, 2, 7, 4, 9, 5};
    process p(5);
    p.execute(data);

    // test 1

    std::vector<int> vec = {1, 3, 7, 9, 2, 4, 6, 8, 5};
    std::sort(vec.begin(), vec.end(), [](int a, int b) -> bool {
        return a > b;
    });

    for (auto t: vec)
        std::cout << t << " ";
    std::cout << std::endl;

    return 0;
}
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

struct Stu
{
    int id;
    std::string name;
};

void print_vec(const std::vector<int> &vec)
{
    for (const auto t: vec)
        std::cout << t << " ";
    puts("");
}

int main()
{
    std::vector<int> vec = {1, 2, 3, 4, 5};

    vec.push_back(6);

    for (auto t: vec)
    {
        printf("%d ", t);
    }
    printf("---");
    puts("");

    vec.pop_back();
    for (auto t: vec)
    {
        std::cout << t << " ";
    }
    std::cout << std::endl;

    int x = 798;
    vec.insert(vec.begin() + 3, x); // 1 2 3 179 ... 
    for (size_t i = 0; i < vec.size(); i ++ )
        printf("%d ", vec[i]);
    printf("\n");

    vec.erase(vec.begin() + 3);

    for (auto it = vec.begin(); it != vec.end(); it ++ )
        std::cout << *it << " ";
    puts("");

    // printf("capacity: %ld\n", vec.capacity());
    // vec.clear();
    // printf("size: %ld\n", vec.size());
    // printf("capacity: %ld\n", vec.capacity());
    
    // {
    //     std::vector<int> empty;
    //     empty.swap(vec);
    // }
    // printf("capacity: %ld\n", vec.capacity());

    vec[2] = 798;
    print_vec(vec);
    vec.at(2) = 857;
    print_vec(vec);

    try
    {
        vec.at(100) = 111;
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }
    
    for (std::vector<int>::iterator it = vec.begin(); it != vec.end(); it ++ )
        if (*it > 0) *it = - *it;
    print_vec(vec);

    vec.assign(4, 100);
    print_vec(vec);

    std::vector<int> vec2 = {1, 2, 3, 4, 5, 6};
    vec.assign(vec2.begin() + 1, vec2.begin() + 4);
    print_vec(vec);

    std::vector<std::vector<int>> matrix(3, std::vector<int>(4, 0));

    std::vector<Stu> svec;

    svec.push_back({1, "jason"});
    svec.push_back({2, "fku"});

    for (const auto& stu: svec)
    {
        printf("%d %s\n", stu.id, stu.name.c_str());
    }

    std::vector<std::string> nvec = {"111", "222", "333"};

    std::string target = "222";
    auto it = std::find(nvec.begin(), nvec.end(), target);

    if(it != nvec.end()) {
        std::cout << target << " found at position " << std::distance(nvec.begin(), it) << std::endl;
    }
    else {
        std::cout << target << " not found." << std::endl;
    }

    std::sort(vec.begin(), vec.end(), [](int a, int b){
        return a > b;
    });

    print_vec(vec);

    std::vector<int> vec3;
    printf("capacity: %ld\n", vec3.capacity());
    vec3.reserve(100);
    printf("capacity: %ld\n", vec3.capacity());

    for (int i = 0; i < 100; i ++ ) vec3.push_back(i);
    printf("capacity: %ld\n", vec3.capacity());
    vec3.clear();
    printf("size: %ld\n", vec3.size());
    printf("capacity: %ld\n", vec3.capacity()); // 100

    vec3.shrink_to_fit();
    printf("capacity: %ld\n", vec3.capacity());

    return 0;
}
#include "umap_demo.h"

struct point
{
    int x, y;
    bool operator==(const point& other) const
    {
        return x == other.x && y == other.y;
    }
};

struct point_hash
{
    size_t operator()(const point& p) const
    {
        size_t h1 = std::hash<int>()(p.x);
        size_t h2 = std::hash<int>()(p.y);
        return h1 ^ (h2 << 1);
    }
};

struct point_equal
{
    bool operator()(const point& p1, const point& p2) const
    {
        return p1.x == p2.x && p1.y == p2.y;
    }
};


int main()
{
    /*
    std::unordered_map<int, std::string> umap = {
        {1, "one"},
        {2, "two"}
    };

    umap[3] = "three";
    umap.insert({4, "four"});
    umap.emplace(5, "five");

    for (auto& pair: umap)
        std::cout << pair.first << ' ' << pair.second << std::endl;
    std::cout << umap.bucket_count() << std::endl;

    std::unordered_map<point, int, point_hash, point_equal> pmap;
    pmap[{1, 2}] = 1;
    pmap[{3, 4}] = 2;
    pmap[{1, 2}] = 3;
    */

    sen_std::unordered_map<int, std::string> sumap;
    sumap.insert(1, "one");
    sumap.insert(2, "two");
    sumap.insert(3, "three");
    for (auto it = sumap.begin(); it != sumap.end(); ++ it)
        std::cout << it->first << " " << it->second << std::endl;
    
    std::cout << "11111"<< std::endl;
    return 0;
}
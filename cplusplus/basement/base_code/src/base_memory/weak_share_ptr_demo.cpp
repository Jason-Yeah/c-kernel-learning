#include <iostream>
#include <memory>
#include <cstdio>

class B;

class A
{
    public:
        std::shared_ptr<B> ptrB;
        A() {}
        ~A() {}
};

class B
{
    public:
        std::weak_ptr<A> ptrA;
        B() {}
        ~B() {}
};

struct FileDeleter
{
    void operator()(FILE* fp)
    {
        std::cout << "close file" << std::endl;
        if(fp) fclose(fp);
    }
};

int main()
{
    {
        auto ap = std::make_shared<A>();
        auto bp = std::make_shared<B>();

        ap->ptrB = bp;
        bp->ptrA = ap;

        std::cout << ap.use_count() << std::endl;
        std::cout << bp->ptrA.use_count() << std::endl;
        std::cout << bp.use_count() << std::endl;
    }

    {
        auto filesp = std::shared_ptr<FILE>(fopen("weak_share_ptr_demo.cpp", "r"), FileDeleter());
        if (filesp)
        {
            std::cout << "file is opened successfully!" << std::endl;
            char buf[256];
            while (fgets(buf, sizeof(buf), filesp.get()) != NULL)
                printf("%s", buf);
            puts("");
        }
    }
    std::cout << "=====================================================" << std::endl;
    {
        auto fileDeleter = [](FILE *fp) {
            if (fp)
            {
                std::cout << "file is opened successfully!" << std::endl;
                fclose(fp);
            }
        };

       std::unique_ptr<FILE, decltype(fileDeleter)> fileptr(fopen("mempool.h", "r"), fileDeleter);
    
        if (fileptr)
        {
            std::cout << "file is opened successfully!" << std::endl;
            char buf[256];
            while (fgets(buf, sizeof(buf), fileptr.get()) != NULL)
                printf("%s", buf);
            puts("");
        }
    }
    // FileDeleter fd;
    // fd(nullptr);

    return 0; 
} 
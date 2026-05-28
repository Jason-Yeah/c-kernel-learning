#include "simple_shared_ptr.h"
#include <memory>

int main()
{
    SimpleSharedPtr<Stu> sp1_stu;
    std::cout << "sp1_stu use_cnt: " << sp1_stu.use_cnt() << std::endl;

    SimpleSharedPtr<Stu> sp2_stu(new Stu("jason", 23));
    std::cout << "sp2_stu use_cnt: " << sp2_stu.use_cnt() << std::endl;

    // SimpleSharedPtr<Stu> sp3_stu(sp2_stu);
    SimpleSharedPtr<Stu> sp3_stu = sp2_stu;
    std::cout << "sp3_stu use_cnt: " << sp3_stu.use_cnt() << std::endl;
    std::cout << "sp2_stu use_cnt: " << sp2_stu.use_cnt() << std::endl;

    sp1_stu = sp3_stu;
    std::cout << "sp1_stu use_cnt: " << sp1_stu.use_cnt() << std::endl;
    std::cout << "sp3_stu use_cnt: " << sp3_stu.use_cnt() << std::endl;

    sp2_stu.reset(new Stu("Tom", 60));
    std::cout << "sp2_stu use_cnt: " << sp2_stu.use_cnt() << std::endl;
    std::cout << "sp1_stu use_cnt: " << sp1_stu.use_cnt() << std::endl;
    std::cout << "sp3_stu use_cnt: " << sp3_stu.use_cnt() << std::endl;

    std::cout << "=====================================================" << std::endl;

    // auto* stup = new Stu();
    // std::shared_ptr<Stu> sharestu (stup);
    // auto sharedp_stu = std::shared_ptr<Stu>(stup);
    auto sharedp_stu = std::make_shared<Stu>("JY", 23);
    auto sp_stu2 = sharedp_stu;
    std::cout << sharedp_stu.use_count() << std::endl;
    std::cout << sp_stu2.use_count() << std::endl;

    return 0;
}
#include "simple_shared_ptr.h"

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

    return 0;
}
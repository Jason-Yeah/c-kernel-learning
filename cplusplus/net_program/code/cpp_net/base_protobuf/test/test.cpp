#include <iostream>
#include "person.pb.h"

using namespace std;

int main()
{
    demo::person p;
    p.set_id(9190);
    p.set_name("Jason");

    std::string serialized;
    p.SerializeToString(&serialized);

    std::cout << serialized.size() << endl;

    demo::person q;
    q.ParseFromString(serialized);

    std::cout << q.id() << endl;
    std::cout << q.name() << endl;

    return 0;
}

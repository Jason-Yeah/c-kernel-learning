#include <iostream>
#include <vector>
#include <memory>

class Vehicle
{
    public:
        //只声明不实现，强制子类实现该函数
        virtual void start_engine() = 0;
        
        virtual ~Vehicle() {}
};

class Car : public Vehicle
{
    public:
        void start_engine() override
        {
            printf("fk\n");
        }
};

class Base {
public:
    int publicMember;
protected:
    int protectedMember;
private:
    int privateMember;
};

class PublicDerived : public Base
{
    public:
        void access_members()
        {
            publicMember = 1;
            protectedMember = 1;
        }
};

class ProtectedDerived : protected Base {
public:
    void access_members() {
        publicMember = 1;      // 转为 protected
        protectedMember = 2;   // 转为 protected
        // privateMember = 3;  // 错误
    }
};

class PrivateDerived : private Base {
public:
    void access_members() {
        publicMember = 1;      // 转为 private
        protectedMember = 2;   // 转为 private
        // privateMember = 3;  // 错误
    }
};

class BASE
{
    public:
        virtual void show () const
        {
            std::cout << "BASE\n";
        }
        virtual ~BASE() {}
};

class CLAZZ : public BASE
{
    public:
        void show () const override
        {
            std::cout << "Derived show" << std::endl;
        }
};

int main()
{
    Car car;
    car.start_engine();

    Vehicle *v1 = &car;
    v1->start_engine();

    PublicDerived pubDer;
    pubDer.publicMember = 10; // 可访问

    std::vector<BASE> vecb;

    CLAZZ clz;
    vecb.push_back(clz);

    vecb[0].show(); // 切片问题

    std::vector<std::unique_ptr<BASE>> vecpb;
    vecpb.emplace_back(std::make_unique<CLAZZ>());

    vecpb[0]->show();

    return 0;
}
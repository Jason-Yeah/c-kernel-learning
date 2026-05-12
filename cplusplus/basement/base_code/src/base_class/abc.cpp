#include <iostream>

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

int main()
{
    Car car;
    car.start_engine();

    Vehicle *v1 = &car;
    v1->start_engine();

    PublicDerived pubDer;
    pubDer.publicMember = 10; // 可访问

    return 0;
}
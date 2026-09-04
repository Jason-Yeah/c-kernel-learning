#include <iostream>
#include <string>
#include <utility>

class Person
{
public:
    explicit Person(std::string name) : name_(std::move(name)) {}
    Person(const Person &rhs) : name_(rhs.name_)
    {
        std::cout << "copy Person\n";
    }

    const std::string &name() const noexcept { return name_; }

private:
    std::string name_;
};

void printByValue(Person person)
{
    std::cout << "by value: " << person.name() << '\n';
}

void printByConstReference(const Person &person)
{
    std::cout << "by const reference: " << person.name() << '\n';
}

class Window
{
public:
    virtual ~Window() = default;
    virtual void display() const { std::cout << "Window\n"; }
};

class SpecialWindow final : public Window
{
public:
    void display() const override { std::cout << "SpecialWindow\n"; }
};

void showByValue(Window window) { window.display(); }
void showByReference(const Window &window) { window.display(); }

int main()
{
    Person ada{"Ada"};
    printByValue(ada);
    printByConstReference(ada);

    SpecialWindow special;
    showByValue(special);     // 切片后调用 Window::display
    showByReference(special); // 保留动态类型，调用 SpecialWindow::display
}

#pragma once

#include <iostream>
#include <memory>
#include <string>

class Resume
{
public:
    virtual ~Resume() = default;
    virtual std::unique_ptr<Resume> clone() const = 0;
    virtual void setExp(const std::string &) = 0;
    virtual void show() const = 0;
};

class ConcreteResume : public Resume
{
public:
    ConcreteResume(const std::string &name, int age,
                   const std::string &experience)
        : name_(name), age_(age), experience_(experience)
    {
        std::cout << "[CONSTRUCT] Creating resume template.\n";
    }

    // Core
    ConcreteResume(const ConcreteResume &other)
        : name_(other.name_ + "duplication"), age_(other.age_),
          experience_(other.experience_)
    {
        std::cout << "[COPY] Cloning resume.\n";
    }

    // Core
    std::unique_ptr<Resume> clone() const override
    {
        return std::make_unique<ConcreteResume>(*this);
    }

    void setExp(const std::string &exp) override { experience_ = exp; }

    void show() const override
    {
        std::cout << "  姓名: " << name_ << "\n"
                  << "  年龄: " << age_ << "\n"
                  << "  经历: "
                  << (experience_.empty() ? "(未填写)" : experience_)
                  << std::endl;
    }

private:
    std::string name_;
    int age_;
    std::string experience_;
};

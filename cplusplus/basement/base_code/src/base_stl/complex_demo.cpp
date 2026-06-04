#include <iostream>

class complex
{
    private:
        double real;
        double imag;
    public:
        complex(double r, double i): real(r), imag(i) {}
        
        complex operator+ (const complex& other) const
        {
            return complex(this->real + other.real, imag + other.imag);
        }

        complex& operator = (const complex& c)
        {
            if (&c == this) return *this;
            real = c.real;
            imag = c.imag;
            return *this;
        }

        // << 左操作数一般是std::ostream而不是this，使用友元形式
        friend std::ostream& operator<< (std::ostream& os, const complex& other);
};

std::ostream& operator<< (std::ostream& os, const complex& other)
{
    os << other.real;
    if (other.imag > 0)
        os << "+" << other.imag << "i";
    if (other.imag < 0)
        os << "-" << other.imag << "i";\
    return os;
}

class logger
{
    private:
        std::string _msg;
    public:
        logger(const std::string& msg = ""): _msg(msg) {}
        logger operator, (const logger& other) const
        {
            return logger(this->_msg + ", " + other._msg);
        }
        friend std::ostream& operator<< (std::ostream& os, const logger& l);
};

std::ostream& operator<< (std::ostream& os, const logger& l)
{
    os << l._msg;
    return os;
}

int main()
{
    complex c1(3, 4);
    complex c2(1.5, -2.5);
    complex c3 = c1 + c2;
    std::cout << "c1 + c2 = " << c3 << std::endl;

    logger l1("start");
    logger l2("load config...");
    logger l3("init...");
    logger combine = (l1, l2, l3);
    
    std::cout << combine << std::endl;

    return 0;
}
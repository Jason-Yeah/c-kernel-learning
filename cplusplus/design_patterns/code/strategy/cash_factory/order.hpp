#pragma once

class Order
{
public:
    Order(double p, int c = 1) : price(p), cnt(c) {}
    double sum() { return price * cnt; }

private:
    double price;
    int cnt;
};

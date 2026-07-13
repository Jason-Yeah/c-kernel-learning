#pragma once

class Operator
{
public:
    Operator() = default;
    virtual ~Operator() = default;
    virtual double get_res() = 0;

    double _num_a = 0;
    double _num_b = 0;
};

class OperatorAdd : public Operator
{
public:
    double get_res() override { return _num_a + _num_b; }
    ~OperatorAdd() = default;
};

class OperatorSub : public Operator
{
public:
    ~OperatorSub() = default;
    double get_res() override { return _num_a - _num_b; }
};

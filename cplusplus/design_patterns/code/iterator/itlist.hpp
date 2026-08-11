#pragma once

#include <memory>

template <typename T> class Iterator
{
public:
    virtual ~Iterator() = default;
    virtual void first() = 0;
    virtual void next() = 0;
    virtual bool is_done() const = 0;
    virtual T current_item() const = 0;
};

template <typename T> class MyList;

template <typename T> class MyListIterator : public Iterator<T>
{
    const MyList<T> &list_;
    int idx_ = 0;

public:
    MyListIterator() = default;

    MyListIterator(const MyList<T> &list) : list_(list) {}

    void first() override { idx_ = 0; }

    void next() override { ++idx_; }

    bool is_done() const override { return idx_ >= list_.count(); }

    T current_item() const override { return list_.get_item(idx_); }
};

#pragma once

template <typename T> class MyList
{
    T *data_ = nullptr;
    int size_ = 0;
    int capacity_ = 0;

public:
    MyList() = default;
    ~MyList() { delete[] data_; }

    void push_back(const T &val)
    {
        if (size_ >= capacity_)
        {
            capacity_ = capacity_ == 0
                            ? 4
                            : capacity_ * 2; // empty let it 4 else 2 times
            T *ndata = new T[capacity_];
            for (size_t i = 0; i < size_; ++i)
                ndata[i] = data_[i];
            delete[] data_;
            data_ = ndata;
        }
        data_[size_++] = val;
    }

    std::unique_ptr<Iterator<T>> create_iterator() const
    {
        return std::make_unique<MyListIterator<T>>(*this);
    }

    size_t count() const { return size_; }

    T get_item(int idx) const { return data_[idx]; }
};

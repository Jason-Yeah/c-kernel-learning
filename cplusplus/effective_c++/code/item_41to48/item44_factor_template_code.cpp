#include <array>
#include <cstddef>
#include <iostream>

template <typename T>
class MatrixCore {
protected:
    // 逻辑只依赖元素类型 T，不依赖矩阵边长 N，因此从 SquareMatrix<T, N>
    // 中提取出来。对相同的 T，不同 N 可以复用这份实现。
    static void makeIdentity(T *data, std::size_t side)
    {
        for (std::size_t row = 0; row < side; ++row) {
            for (std::size_t column = 0; column < side; ++column) {
                data[row * side + column] = (row == column) ? T{1} : T{0};
            }
        }
    }
};

template <typename T, std::size_t N>
class SquareMatrix : private MatrixCore<T> {
public:
    void setIdentity()
    {
        MatrixCore<T>::makeIdentity(values_.data(), N);
    }

    void print() const
    {
        for (std::size_t row = 0; row < N; ++row) {
            for (std::size_t column = 0; column < N; ++column) {
                std::cout << values_[row * N + column] << ' ';
            }
            std::cout << '\n';
        }
    }

private:
    std::array<T, N * N> values_{};
};

int main()
{
    SquareMatrix<double, 2> matrix2;
    SquareMatrix<double, 3> matrix3;

    matrix2.setIdentity();
    matrix3.setIdentity();

    std::cout << "2 x 2:\n";
    matrix2.print();
    std::cout << "3 x 3:\n";
    matrix3.print();
}

#ifndef RYFMACH_BEL_LANG_ENGINE_INCLUDE_UTILS_MATRIX_VIEW_HPP_
#define RYFMACH_BEL_LANG_ENGINE_INCLUDE_UTILS_MATRIX_VIEW_HPP_

#include <cstdint>
#include <span>

namespace ryfmach::utils {

template <typename T>
class MatrixSpan2d {
    std::span<T> content_;
    std::size_t rows_;
    std::size_t cols_;
    std::size_t stride_;

public:
    MatrixSpan2d() = default;

    MatrixSpan2d(std::span<T> content, std::size_t rows,
                 std::size_t cols, std::size_t stride)
        : content_(content)
        , rows_(rows)
        , cols_(cols)
        , stride_(stride) {}

    template <std::size_t M>
    MatrixSpan2d(T (*content)[M], std::size_t rows, std::size_t cols)
        : content_(&content[0][0], rows * M)
        , rows_(rows)
        , cols_(cols)
        , stride_(M) {}

    MatrixSpan2d(const MatrixSpan2d& other) = default;
    MatrixSpan2d(MatrixSpan2d&& other) = default;
    MatrixSpan2d& operator=(const MatrixSpan2d& other) = default;
    MatrixSpan2d& operator=(MatrixSpan2d&& other) = default;

    T& operator[](std::size_t r, std::size_t c) {
        return content_[r * stride_ + c];
    }
};

} // namespace ryfmach::utils

#endif // RYFMACH_BEL_LANG_ENGINE_INCLUDE_UTILS_MATRIX_VIEW_HPP_

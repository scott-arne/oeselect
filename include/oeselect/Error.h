// include/oeselect/Error.h
#ifndef OESELECT_ERROR_H
#define OESELECT_ERROR_H

#include <stdexcept>
#include <string>

namespace OESel {

/// Exception thrown for invalid selection syntax
class SelectionError : public std::runtime_error {
public:
    explicit SelectionError(const std::string& message)
        : std::runtime_error(message) {}

    SelectionError(const std::string& message, size_t position)
        : std::runtime_error(message), position_(position) {}

    size_t position() const { return position_; }

private:
    size_t position_ = 0;
};

}  // namespace OESel

#endif  // OESELECT_ERROR_H

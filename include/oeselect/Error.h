/**
 * @file Error.h
 * @brief Exception types for selection parsing and evaluation errors.
 */

#ifndef OESELECT_ERROR_H
#define OESELECT_ERROR_H

#include <stdexcept>
#include <string>

namespace OESel {

/**
 * @brief Exception thrown when selection parsing or validation fails.
 *
 * This exception is thrown by OESelection::Parse() when the input string
 * contains invalid syntax. The optional position() method indicates where
 * in the input string the error occurred.
 *
 * @code
 * try {
 *     auto sele = OESelection::Parse("invalid_keyword foo");
 * } catch (const SelectionError& e) {
 *     std::cerr << "Parse error at position " << e.position()
 *               << ": " << e.what() << "\n";
 * }
 * @endcode
 */
class SelectionError : public std::runtime_error {
public:
    /**
     * @brief Construct error with message only.
     * @param message Descriptive error message.
     */
    explicit SelectionError(const std::string& message)
        : std::runtime_error(message) {}

    /**
     * @brief Construct error with message and position.
     * @param message Descriptive error message.
     * @param position Character offset in input where error occurred.
     */
    SelectionError(const std::string& message, const size_t position)
        : std::runtime_error(message), position_(position) {}

    /**
     * @brief Get the position in the input string where parsing failed.
     * @return Zero-based character offset, or 0 if not applicable.
     */
    [[nodiscard]] size_t position() const { return position_; }

private:
    size_t position_ = 0;  ///< Character offset of error in input
};

}  // namespace OESel

#endif  // OESELECT_ERROR_H

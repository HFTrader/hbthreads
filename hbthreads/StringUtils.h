// StringUtils.h - String manipulation and formatting utilities
//
// This header provides utility functions for string formatting and output,
// particularly focused on hexadecimal data display and numeric string conversion.
// These utilities are designed for debugging and logging in high-performance
// applications where efficient string operations are important.

#pragma once
#include <cstdint>
#include <ostream>
#include <string>

namespace hbthreads {

// Print binary data as hexadecimal with optional line prefix and formatting
// Outputs data in hex format with configurable items per line and line prefixes.
// Useful for debugging binary protocols, memory dumps, and data inspection.
//
// Parameters:
//   out - Output stream to write to
//   data - Pointer to binary data to display
//   size - Number of bytes to display
//   line_prefix - String to prepend to each output line (e.g., "DEBUG: ")
//   NUMITEMS - Number of hex bytes to display per line
void printhex(std::ostream& out, const void* data, uint32_t size,
              const std::string& line_prefix, int NUMITEMS);

// Convert uint32_t value to decimal string with zero-padding
// Converts a numeric value to its decimal string representation with fixed width.
// Pads with leading zeros to ensure exactly N digits in the output.
//
// Template parameter:
//   N - Number of digits to output (must be >= 1)
//
// Parameters:
//   ptr - Output buffer to write digits to (must have space for N+1 chars)
//   value - Numeric value to convert (values larger than 10^N-1 will overflow)
//
// Returns:
//   Pointer to character after the last written digit
//
// Example: printpad<3>(buffer, 42) writes "042" and returns buffer + 3
template <size_t N>
char* printpad(char* ptr, uint32_t value) {
    for (size_t j = 0; j < N; ++j) {
        ptr[N - j - 1] = (value % 10) + '0';
        value = value / 10;
    }
    return ptr + N;
}

}  // namespace hbthreads
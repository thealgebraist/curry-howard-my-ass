#pragma once

#include <array>
#include <cstdint>

// Source transcription of html_entity_parser.cc's kWindowsLatin1ExtensionArray.
// The λΠ model keeps a separate table so this value table can serve as the
// independent C++ observation in the finite numeric correspondence.
namespace blink_numeric_entity_table {

inline constexpr std::array<uint32_t, 32> windows_latin1_extension = {
    0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x008d, 0x017d, 0x008f,
    0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178};

constexpr uint32_t adjust(uint32_t value) {
  return value >= 0x80 && value <= 0x9f
             ? windows_latin1_extension[value - 0x80]
             : value;
}

}  // namespace blink_numeric_entity_table

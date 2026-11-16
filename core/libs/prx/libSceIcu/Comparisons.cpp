#include "prx/libSceIcu/libSceIcu.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"
#include "prx/libc/include/General.hpp"
#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

namespace {

void CopyIn(void* destination, const void* source, std::size_t bytes, const char* caller) {
    const int savedErrno = errno;
    const bool ok = GuestCopyIn_nid_no_patch(destination, source, bytes);
    errno = savedErrno;
    if (!ok) NotImplemented_nid_no_patch(caller);
}

constexpr std::int32_t ChunkUnits = 64;

std::vector<char16_t> CopyWindow(const char16_t* text, std::int32_t count, const char* caller) {
    const std::int32_t size = std::min(count, ChunkUnits);
    std::vector<char16_t> units(static_cast<std::size_t>(size));
    if (size > 0) CopyIn(units.data(), text, units.size() * sizeof(char16_t), caller);
    return units;
}

class UnitReader {
public:
    UnitReader(const char16_t* address, const char* caller) : address(address), caller(caller) {}

    char16_t At(std::size_t index) {
        if (index == cachedIndex) return cached;
        char16_t unit = 0;
        CopyIn(&unit, address + index, sizeof(unit), caller);
        cachedIndex = index;
        cached = unit;
        return unit;
    }

private:
    const char16_t* address;
    const char* caller;
    std::size_t cachedIndex = std::numeric_limits<std::size_t>::max();
    char16_t cached = 0;
};

bool Lead(char16_t value) {
    return value >= 0xd800 && value <= 0xdbff;
}

bool Trail(char16_t value) {
    return value >= 0xdc00 && value <= 0xdfff;
}

std::int32_t Length(const char16_t* text, const char* caller) {
    UnitReader reader(text, caller);
    for (std::int32_t length = 0;; ++length) {
        if (reader.At(static_cast<std::size_t>(length)) == 0) return length;
        if (length == std::numeric_limits<std::int32_t>::max()) throw std::overflow_error("ICU: string length exceeds int32");
    }
}

bool Paired(const char16_t* text, std::size_t length, std::size_t index, const char* caller) {
    UnitReader reader(text, caller);
    const char16_t current = reader.At(index);
    if (Lead(current) && index + 1 < length && Trail(reader.At(index + 1))) return true;
    return Trail(current) && index != 0 && Lead(reader.At(index - 1));
}

void RequireBuffers(const char16_t* first, const char16_t* second) {
    if (first == nullptr || second == nullptr) throw std::invalid_argument("ICU: null comparison buffer");
}

} // namespace

extern "C" {

std::int32_t APS5_VABI u_strlen_nid_postfix(const char16_t* text) {
    if (text == nullptr) throw std::invalid_argument("ICU: null string");
    return Length(text, __func__);
}

std::int32_t APS5_VABI u_strcmp_nid_postfix(const char16_t* first, const char16_t* second) {
    RequireBuffers(first, second);
    UnitReader left(first, __func__);
    UnitReader right(second, __func__);
    for (std::size_t index = 0;; ++index) {
        const char16_t leftUnit = left.At(index);
        const auto difference = static_cast<std::int32_t>(leftUnit) - static_cast<std::int32_t>(right.At(index));
        if (difference != 0 || leftUnit == 0) return difference;
    }
}

std::int32_t APS5_VABI u_strncmp_nid_postfix(const char16_t* first, const char16_t* second, std::int32_t count) {
    if (count <= 0) return 0;
    RequireBuffers(first, second);
    for (std::int32_t base = 0; base < count;) {
        const auto left = CopyWindow(first + base, count - base, __func__);
        const auto right = CopyWindow(second + base, count - base, __func__);
        for (std::size_t index = 0; index < left.size(); ++index) {
            const auto difference = static_cast<std::int32_t>(left[index]) - static_cast<std::int32_t>(right[index]);
            if (difference != 0 || left[index] == 0) return difference;
        }
        base += static_cast<std::int32_t>(left.size());
    }
    return 0;
}

std::int32_t APS5_VABI u_memcmp_nid_postfix(const char16_t* first, const char16_t* second, std::int32_t count) {
    if (count <= 0) return 0;
    RequireBuffers(first, second);
    for (std::int32_t base = 0; base < count;) {
        const auto left = CopyWindow(first + base, count - base, __func__);
        const auto right = CopyWindow(second + base, count - base, __func__);
        for (std::size_t index = 0; index < left.size(); ++index) {
            const auto difference = static_cast<std::int32_t>(left[index]) - static_cast<std::int32_t>(right[index]);
            if (difference != 0) return difference;
        }
        base += static_cast<std::int32_t>(left.size());
    }
    return 0;
}

std::int32_t APS5_VABI u_memcmpCodePointOrder_nid_postfix(const char16_t* first, const char16_t* second, std::int32_t count) {
    if (count == 0 || first == second) return 0;
    RequireBuffers(first, second);
    const std::size_t firstLength = count < 0 ? static_cast<std::size_t>(Length(first, __func__)) + 1 : static_cast<std::size_t>(count);
    const std::size_t secondLength = count < 0 ? static_cast<std::size_t>(Length(second, __func__)) + 1 : static_cast<std::size_t>(count);
    const std::size_t common = std::min(firstLength, secondLength);
    UnitReader left(first, __func__);
    UnitReader right(second, __func__);
    for (std::size_t index = 0; index < common; ++index) {
        std::int32_t leftUnit = left.At(index);
        std::int32_t rightUnit = right.At(index);
        if (leftUnit == rightUnit) continue;
        if (leftUnit >= 0xd800 && rightUnit >= 0xd800) {
            if (!Paired(first, firstLength, index, __func__)) leftUnit -= 0x2800;
            if (!Paired(second, secondLength, index, __func__)) rightUnit -= 0x2800;
        }
        return leftUnit - rightUnit;
    }
    return 0;
}

}

#pragma once

#include <functional>
#include <iosfwd>

namespace opossum {

/**
 * @brief Represents SQL null value in AllTypeVariant
 *
 * Based on boost/blank.hpp with changed relational operators
 */
struct NullValue {};

// Relational operators
inline bool operator==(const NullValue&, const NullValue&) { return false; }
inline bool operator!=(const NullValue&, const NullValue&) { return false; }
inline bool operator<(const NullValue&, const NullValue&) { return false; }
inline bool operator<=(const NullValue&, const NullValue&) { return false; }
inline bool operator>(const NullValue&, const NullValue&) { return false; }
inline bool operator>=(const NullValue&, const NullValue&) { return false; }
inline NullValue operator-(const NullValue&) { return NullValue{}; }

inline size_t hash_value(const NullValue&) {
  // Aggregate wants all NULLs in one bucket
  return 0;
}

}  // namespace opossum

namespace std {
  template <> struct hash<opossum::NullValue>
  {
    size_t operator()(const opossum::NullValue & x) const {
    	return 0;
    }
  };

std::ostream& operator<<(std::ostream&, const opossum::NullValue&);
}

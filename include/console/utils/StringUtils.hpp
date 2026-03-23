#pragma once

#include "console/common/Types.hpp"

namespace console {

Vector<String> split_string(const String& str, char delimiter);
String trim(const String& str);
String to_lower(String str);
String to_upper(String str);

} // namespace console

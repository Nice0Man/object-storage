#include "console/common/Types.hpp"

#include <algorithm>
#include <sstream>

namespace console {

Vector<String>
split_string(const String& str, char delimiter) {
    Vector<String> tokens;
    std::stringstream ss(str);
    String token;

    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }

    return tokens;
}

String
trim(const String& str) {
    auto start = str.find_first_not_of(" \t\n\r");
    if (start == String::npos) {
        return "";
    }

    auto end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

String
to_lower(String str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });
    return str;
}

String
to_upper(String str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::toupper(c); });
    return str;
}

} // namespace console

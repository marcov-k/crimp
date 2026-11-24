#pragma once

#include <string>
#include <string_view>

bool CheckTextsEqual(std::string_view expected, std::string_view actual, std::string& message);
#pragma once

#include <string>
#include <vector>
#include "nlohmann/json.hpp"

class TranslationManager {
public:
    TranslationManager(std::string locale = "en", const std::string& locales_dir = "locales");
    std::string translate(const std::string& key);

private:
    std::string locales_dir;
    std::string locale;
    bool locale_not_found;
    nlohmann::json translations;

    nlohmann::json load_translations(const std::string& locale);
    nlohmann::json load_locale_file(const std::string& locale_name);
    std::vector<std::string> split_locale(const std::string& locale);
};
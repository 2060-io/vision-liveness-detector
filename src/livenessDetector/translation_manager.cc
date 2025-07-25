#include "translation_manager.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

TranslationManager::TranslationManager(std::string locale, const std::string& locales_dir)
    : locales_dir(locales_dir), locale(locale), locale_not_found(false) 
{
    translations = load_translations(locale);
}

TranslationManager::TranslationManager(std::string locale, const std::vector<std::string>& locales_dirs)
    : locales_dir(""), locales_dirs(locales_dirs), locale(locale), locale_not_found(false)
{
    translations = load_translations(locale);
}

std::string TranslationManager::translate(const std::string& key) {
    if (locale_not_found) {
        return key;
    }

    nlohmann::json current = translations;
    std::istringstream iss(key);
    std::string token;

    while (std::getline(iss, token, '.')) {
        try {
            current = current.at(token);
        } catch (nlohmann::json::exception& e) {
            return key; // Return the key if not found
        }
    }

    if (current.is_string()) {
        return current.get<std::string>();
    }

    return key; // Return the key if not found
}

nlohmann::json TranslationManager::load_translations(const std::string& locale) {
    std::cout << "Loading translation from:" << locale << std::endl;
    auto parts = split_locale(locale);
    try {
        return load_locale_file(locale);
    } catch (std::exception&) {
        if (parts.size() > 1) {
            try {
                return load_locale_file(parts[0]);
            } catch (std::exception&) {
                std::cerr << "Warning, Locale Part not found: " << locale << std::endl;
            }
        }
        try {
            return load_locale_file("default");
        } catch (std::exception&) {
            locale_not_found = true;
            std::cerr << "Warning, Locale not found: " << locale << std::endl;
            return nlohmann::json::object();
        }
    }
}

nlohmann::json TranslationManager::load_locale_file(const std::string& locale_name) {
    nlohmann::json merged_json = nlohmann::json::object();
    bool found_any = false;

    if (!locales_dirs.empty()) {
        for (const auto& dir : locales_dirs) {
            std::string filename = (std::filesystem::path(dir) / (locale_name + ".json")).string();
            std::ifstream f(filename);
            if (f) {
                nlohmann::json j;
                f >> j;
                merged_json.update(j);  // merge new keys/values, overwrite previous if overlapping
                found_any = true;
            }
        }
        if (!found_any) {
            throw std::runtime_error("Locale file not found in provided local_dirs.");
        }
        return merged_json;
    }

    // Single legacy directory fallback
    std::string filename = (std::filesystem::path(locales_dir) / (locale_name + ".json")).string();
    std::ifstream f(filename);
    if (!f) {
        throw std::runtime_error("Locale file not found");
    }
    nlohmann::json j;
    f >> j;
    return j;
}

std::vector<std::string> TranslationManager::split_locale(const std::string& locale) {
    std::vector<std::string> parts;
    std::istringstream iss(locale);
    std::string token;
    while (std::getline(iss, token, '_')) {
        parts.push_back(token);
    }
    return parts;
}
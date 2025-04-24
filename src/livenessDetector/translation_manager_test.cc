#include <iostream>
#include "translation_manager.h"
#include <filesystem>
#include <fstream>

int main() {
    // Set up a test locale directory and default JSON files
    const std::string locales_dir = "locales";
    std::filesystem::create_directory(locales_dir);

    {
        // Create a demo JSON file for "en.json"
        std::ofstream out_en(locales_dir + "/en.json");
        out_en << R"({
            "greeting": {
                "hello": "Hello, World!",
                "hi": "Hi there!"
            }
        })";
        out_en.close();
    }

    {
        // Create a demo JSON file for "default.json"
        std::ofstream out_default(locales_dir + "/default.json");
        out_default << R"({
            "greeting": {
                "hello": "Hello!"
            }
        })";
        out_default.close();
    }

    // Create a TranslationManager instance
    TranslationManager manager("en", locales_dir);

    // Test translating a key
    std::string translatedHello = manager.translate("greeting.hello");
    std::cout << "Translated 'greeting.hello': " << translatedHello << std::endl;

    std::string translatedHi = manager.translate("greeting.hi");
    std::cout << "Translated 'greeting.hi': " << translatedHi << std::endl;

    std::string untranslatedKey = manager.translate("nonexistent.key");
    std::cout << "Untranslated 'nonexistent.key': " << untranslatedKey << std::endl;

    return 0;
}
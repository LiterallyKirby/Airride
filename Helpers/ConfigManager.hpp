#pragma once

#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

class ConfigManager {
private:
  std::string prompt;
  std::string bgColor;
  std::string textColor;
  YAML::Node config; // <--- Add this line

public:
  // Constructor
  ConfigManager(const std::string &configFile);

  // Getters
  std::string getPrompt() const;
  std::string getBgColor() const;
  std::string getTextColor() const;

  // Returns the prompt string with colors and placeholder replacements
  std::string getFormattedPrompt() const;

  // Utility function to format any text with fg/bg colors
  std::string formatSection(const std::string &text, const std::string &fg,
                            const std::string &bg) const;
};

// Optional: Declare color maps here if you want to share them externally
extern const std::unordered_map<std::string, std::string> colorCodes;
extern const std::unordered_map<std::string, std::string> bgColorCodes;

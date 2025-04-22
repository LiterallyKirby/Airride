#ifndef CONFIGMANAGER_HPP
#define CONFIGMANAGER_HPP

#include <string>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

class ConfigManager {
public:
  explicit ConfigManager(const std::string &configPath);
  std::string getFormattedPrompt();
  std::string ReplacePlaceholders(std::string prompt);

private:
  static const std::unordered_map<std::string, std::string> colorCodes;
  static const std::unordered_map<std::string, std::string> bgColorCodes;
  std::string configPath;
  YAML::Node config;

  void loadConfig();
};

#endif // CONFIGMANAGER_HPP

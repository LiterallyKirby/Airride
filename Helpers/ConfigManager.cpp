#include "ConfigManager.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits.h> // Include for PATH_MAX
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

// Initialize static color maps
const std::unordered_map<std::string, std::string> ConfigManager::colorCodes = {
    {"black", "0;30"},  {"red", "0;31"},   {"green", "0;32"},
    {"yellow", "0;33"}, {"blue", "0;34"},  {"magenta", "0;35"},
    {"cyan", "0;36"},   {"white", "0;37"}, {"bright_white", "1;37"},
};

const std::unordered_map<std::string, std::string> ConfigManager::bgColorCodes =
    {
        {"black", "40"}, {"red", "41"},     {"green", "42"}, {"yellow", "43"},
        {"blue", "44"},  {"magenta", "45"}, {"cyan", "46"},  {"white", "47"},
};

ConfigManager::ConfigManager(const std::string &configPath)
    : configPath(configPath) { // Initialize configPath
  loadConfig();
}

void ConfigManager::loadConfig() {
  try {
    config = YAML::LoadFile(configPath);
  } catch (const std::exception &e) {
    std::cerr << "Error loading config: " << e.what() << std::endl;
  }
}

std::string ConfigManager::ReplacePlaceholders(std::string prompt) {
  // Get current working directory
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd failed");
    return prompt;
  }

  // Get hostname
  char hostname[256];
  gethostname(hostname, sizeof(hostname));

  // Get username
  const char *username = getenv("USER");
  if (!username)
    username = getpwuid(getuid())->pw_name;

  size_t pos;
  while ((pos = prompt.find("{cwd}")) != std::string::npos)
    prompt.replace(pos, 5, cwd);
  while ((pos = prompt.find("{host}")) != std::string::npos)
    prompt.replace(pos, 6, hostname);
  while ((pos = prompt.find("{user}")) != std::string::npos)
    prompt.replace(pos, 6, username);

  return prompt;
}

std::string ConfigManager::getFormattedPrompt() {
  std::string result;

  if (!config["prompt"])
    return "$ ";

  for (const auto &part : config["prompt"]) {
    std::string text = part["text"].as<std::string>("");
    std::string fg = part["fg"].as<std::string>("white");
    std::string bg = part["bg"].as<std::string>("reset");

    // Call ReplacePlaceholders here
    text = ReplacePlaceholders(text);

    std::string fgCode =
        colorCodes.find(fg) != colorCodes.end() ? colorCodes.at(fg) : "";
    std::string bgCode =
        bgColorCodes.find(bg) != bgColorCodes.end() ? bgColorCodes.at(bg) : "";

    if (!fgCode.empty() || !bgCode.empty()) {
      result += "\033[";
      if (!fgCode.empty())
        result += fgCode;
      if (!fgCode.empty() && !bgCode.empty())
        result += ";";
      if (!bgCode.empty())
        result += bgCode;
      result += "m";
    }

    result += text;

    if (!fgCode.empty() || !bgCode.empty()) {
      result += "\033[0m";
    }
  }

  return result;
}

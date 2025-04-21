#include "ConfigManager.hpp"
#include <fstream>
#include <iostream>
#include <yaml-cpp/yaml.h>

// Define your color maps (you can move these to a better location later)
const std::unordered_map<std::string, std::string> colorCodes = {
    {"black", "30"}, {"red", "31"},     {"green", "32"}, {"yellow", "33"},
    {"blue", "34"},  {"magenta", "35"}, {"cyan", "36"},  {"white", "37"}};

const std::unordered_map<std::string, std::string> bgColorCodes = {
    {"black", "40"}, {"red", "41"},     {"green", "42"}, {"yellow", "43"},
    {"blue", "44"},  {"magenta", "45"}, {"cyan", "46"},  {"white", "47"}};

ConfigManager::ConfigManager(const std::string &configFile) {
  try {
    config =
        YAML::LoadFile(configFile); // <-- now saving to the member variable

    if (config["prompt"] && config["prompt"].IsScalar()) {
      prompt = config["prompt"].as<std::string>();
    }
    if (config["bg_color"]) {
      bgColor = config["bg_color"].as<std::string>();
    }
    if (config["text_color"]) {
      textColor = config["text_color"].as<std::string>();
    }
  } catch (const std::exception &e) {
    std::cerr << "Failed to load config file: " << e.what() << std::endl;
  }
}

std::string ConfigManager::getPrompt() const {
  return prompt.empty() ? "$ " : prompt;
}

std::string ConfigManager::getBgColor() const {
  return bgColor.empty() ? "black" : bgColor;
}

std::string ConfigManager::getTextColor() const {
  return textColor.empty() ? "white" : textColor;
}

extern std::string
ReplacePlaceholders(std::string prompt); // Linker will look for this elsewhere

std::string ConfigManager::getFormattedPrompt() const {
  std::stringstream formattedPrompt;

  try {
    YAML::Node promptSections = config["prompt"];
    if (promptSections && promptSections.IsSequence()) {
      for (const auto &section : promptSections) {
        std::string text = section["text"].as<std::string>("");
        std::string fg = section["fg"].as<std::string>("white");
        std::string bg = section["bg"].as<std::string>("black");

        text = ReplacePlaceholders(text);
        formattedPrompt << formatSection(text, fg, bg);
      }
    } else {
      // Fallback to simple prompt if sections aren't defined
      std::string promptText = getPrompt();
      promptText = ReplacePlaceholders(promptText);
      formattedPrompt << formatSection(promptText, getTextColor(),
                                       getBgColor());
    }
  } catch (const YAML::Exception &e) {
    std::cerr << "Error processing prompt config: " << e.what() << std::endl;
    // Fallback to simple prompt
    std::string promptText = getPrompt();
    promptText = ReplacePlaceholders(promptText);
    formattedPrompt << formatSection(promptText, getTextColor(), getBgColor());
  }

  return formattedPrompt.str();
}

std::string ConfigManager::formatSection(const std::string &text,
                                         const std::string &fg,
                                         const std::string &bg) const {
  auto fgIt = colorCodes.find(fg);
  auto bgIt = bgColorCodes.find(bg);

  std::string fgCode =
      fgIt != colorCodes.end() ? fgIt->second : "39"; // Default
  std::string bgCode =
      bgIt != bgColorCodes.end() ? bgIt->second : "49"; // Default

  return "\033[" + fgCode + ";" + bgCode + "m" + text + "\033[0m";
}

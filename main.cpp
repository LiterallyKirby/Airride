#include "Helpers/ConfigManager.hpp"
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <pwd.h>
#include <regex>
#include <sstream>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <yaml-cpp/yaml.h>

std::deque<std::string> commandHistory;
size_t historyPosition = 0;
std::string currentInput;

using namespace std;
namespace fs = std::filesystem;

int visibleLength(const std::string &str) {
  return std::regex_replace(str, std::regex(R"(\x1b\[[0-9;]*m)"), "").length();
}

void BuildTree(const fs::path &path, int depth = 1, bool isLast = false,
               int max_depth = -1) {
  if (!fs::exists(path)) {
    std::cerr << "Path does not exist: " << path << "\n";
    return;
  }

  if (max_depth >= 0 && depth > max_depth)
    return;

  std::string indent;
  for (int i = 0; i < depth; ++i)
    indent += (i == depth - 1) ? (isLast ? "    " : "│   ") : "    ";

  try {
    std::vector<fs::directory_entry> entries;
    for (const auto &entry : fs::directory_iterator(path)) {
      entries.push_back(entry);
    }

    for (size_t i = 0; i < entries.size(); ++i) {
      const auto &entry = entries[i];
      bool lastEntry = (i == entries.size() - 1);
      bool atMaxDepth = (max_depth >= 0 && depth == max_depth);

      std::cout << indent << (lastEntry ? "╰──" : "├── ")
                << entry.path().filename().string();

      if (atMaxDepth && fs::is_directory(entry.status())) {
        std::cout << " [content truncated]";
      }
      std::cout << "\n";

      if (fs::is_directory(entry.status()) && !atMaxDepth) {
        BuildTree(entry.path(), depth + 1, lastEntry, max_depth);
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::cerr << "Error accessing " << path << ": " << e.what() << "\n";
  }
}

void createConfigFileIfNotExist() {
  std::filesystem::path configDir =
      std::filesystem::path(getenv("HOME")) / ".config" / "airide";
  std::filesystem::path configFilePath = configDir / "config.yaml";

  if (!std::filesystem::exists(configDir)) {
    std::cout << "Creating config directory: " << configDir << std::endl;
    std::filesystem::create_directories(configDir);
  }

  if (!std::filesystem::exists(configFilePath)) {
    std::cout << "Config file not found. Creating default config..."
              << std::endl;

    YAML::Node config;
    config["setting1"] = "default_value_1";
    config["setting2"] = "default_value_2";
    config["setting3"] = 100;

    std::ofstream out(configFilePath);
    out << config;
    std::cout << "Config file created with default settings." << std::endl;
  } else {
    std::cout << "Config file already exists." << std::endl;
  }
}

std::string ReplacePlaceholders(std::string prompt) {
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd failed");
    return prompt;
  }

  char hostname[256];
  gethostname(hostname, sizeof(hostname));

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

void setRawMode() {
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &= ~(ICANON | ECHO);
  term.c_iflag &= ~(ICRNL);
  term.c_oflag |= OPOST;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void resetTerminal() {
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |= (ICANON | ECHO | ISIG);
  term.c_iflag |= (ICRNL);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

array<string, 9> BuiltIns = {"cd", "ls", "echo", "pwd"};

void HandleCommands(string command) {
  istringstream stream(command);
  string arg;
  vector<string> args;

  while (stream >> arg)
    args.push_back(arg);

  if (args.empty())
    return;

  bool isBuiltIn =
      find(BuiltIns.begin(), BuiltIns.end(), args[0]) != BuiltIns.end();

  if (isBuiltIn) {
    if (args[0] == "cd") {
      if (args.size() < 2) {
        const char *home = getenv("HOME");
        if (home)
          chdir(home);
      } else if (chdir(args[1].c_str()) != 0) {
        perror("cd failed");
      }
    } else if (args[0] == "echo") {
      for (size_t i = 1; i < args.size(); ++i)
        cout << args[i] << " ";
      cout << endl;
    } else if (args[0] == "pwd") {
      char *cwd = getcwd(NULL, 0);
      cout << "\n" << cwd << endl;
      free(cwd);
    } else if (args[0] == "ls") {
      int depth = -1;
      fs::path target = ".";

      for (size_t i = 1; i < args.size(); ++i) {
        if (args[i] == "--depth" || args[i] == "-d") {
          if (i + 1 < args.size()) {
            try {
              depth = std::stoi(args[i + 1]);
              i++;
            } catch (...) {
              std::cerr << "Invalid depth value\n";
              return;
            }
          } else {
            std::cerr << "Missing depth value\n";
            return;
          }
        } else {
          target = args[i];
        }
      }

      if (!fs::exists(target)) {
        std::cout << "Directory does not exist." << std::endl;
        return;
      }

      BuildTree(target, 0, false, depth);
    }
  } else {
    system(command.c_str());
  }
}

size_t cursorPosition = 0;
std::string readInput(ConfigManager &configManager) {
  cursorPosition = 0;
  currentInput.clear();
  char ch;
  std::cout << "\033[0m";

  while (true) {
    ch = getchar();

    if (ch == '\033') {
      char next = getchar();
      if (next == '[') {
        char arrow = getchar();
        switch (arrow) {
        case 'A':
          if (!commandHistory.empty() &&
              historyPosition < commandHistory.size()) {
            std::cout << "\033[2K\r" << configManager.getFormattedPrompt();
            historyPosition++;
            currentInput =
                commandHistory[commandHistory.size() - historyPosition];
            std::cout << currentInput;
          }
          continue;
        case 'B':
          if (historyPosition > 0) {
            std::cout << "\033[2K\r" << configManager.getFormattedPrompt();
            historyPosition--;
            if (historyPosition > 0) {
              currentInput =
                  commandHistory[commandHistory.size() - historyPosition];
            } else {
              currentInput.clear();
            }
            std::cout << currentInput;
          }
          continue;
        case 'D':
          if (cursorPosition > 0) {
            cursorPosition--;
            std::cout << "\033[1D";
          }
          continue;
        case 'C':
          if (cursorPosition < currentInput.length()) {
            cursorPosition++;
            std::cout << "\033[1C";
          }
          continue;
        default:
          continue;
        }
      }
      continue;
    }

    if (ch == '\n' || ch == '\r') {
      std::cout << "\033[0m" << std::endl;
      break;
    }
    if (ch == 127) {
      if (!currentInput.empty()) {
        currentInput.pop_back();
        std::cout << "\b \b";
      }
    } else if (isprint(ch)) {
      currentInput.insert(cursorPosition, 1, ch);
      cursorPosition++;

      std::cout << "\r\033[2K" << configManager.getFormattedPrompt()
                << currentInput;
      std::cout << "\r\033["
                << (visibleLength(configManager.getFormattedPrompt()) +
                    cursorPosition - 6)
                << "C";
    }
  }
  cursorPosition = currentInput.length();
  return currentInput;
}

void clearLine() { std::cout << "\r\033[K"; }

void reprintPrompt(const std::string &prompt) {
  clearLine();
  std::cout << prompt;
}

int main() {
  setRawMode();
  std::cout << "\033[6 q"; // Set cursor to steady bar

  // Ensure config file is created
  createConfigFileIfNotExist();

  std::string homeDir = std::getenv("HOME");
  std::filesystem::path configPath = homeDir + "/.config/airide/config.yaml";
  ConfigManager configManager(configPath);

  while (true) {
    std::cout << configManager.getFormattedPrompt();
    std::string input = readInput(configManager);

    if (input.empty()) {
      continue;
    }

    commandHistory.push_back(input);
    historyPosition = 0;
    HandleCommands(input);
  }

  resetTerminal();
  return 0;
}

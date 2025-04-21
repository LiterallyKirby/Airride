#include "Helpers/ConfigManager.hpp"
#include <array>
#include <cstdlib>
#include <cstring>
#include <fstream> // Add this include
#include <iostream>
#include <limits.h>
#include <pwd.h>
#include <sstream>
#include <stdio.h>
#include <string>
#include <termios.h>
#include <unistd.h>
#include <vector>

#include <yaml-cpp/yaml.h>
using namespace std;

string input;

bool createConfigFile(const std::string &filename) {
  // Check if the config file exists
  std::ifstream file(filename);
  if (file.good()) {
    std::cout << "Config file already exists." << std::endl;
    return false; // File exists, no need to create it
  }

  // If file doesn't exist, create it with default settings
  YAML::Node config;
  config["setting1"] = "default_value_1";
  config["setting2"] = "default_value_2";
  config["setting3"] = 100;

  // Write the default config to the file
  std::ofstream out(filename);
  out << config;
  std::cout << "Config file created with default settings." << std::endl;
  return true; // Successfully created the config file
}
std::string ReplacePlaceholders(std::string prompt) {
  // Get current working directory
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd failed");
    return prompt; // Return prompt as is in case of failure
  }

  char hostname[256];
  gethostname(hostname, sizeof(hostname));

  const char *username = getenv("USER");
  if (!username) {
    username = getpwuid(getuid())->pw_name;
  }

  size_t pos;
  // Replace {cwd} with the actual current working directory
  while ((pos = prompt.find("{cwd}")) != std::string::npos)
    prompt.replace(pos, 5, cwd); // Replace {cwd} with cwd

  // Replace {host} and {user} similarly
  while ((pos = prompt.find("{host}")) != std::string::npos)
    prompt.replace(pos, 6, hostname); // Replace {host} with hostname

  while ((pos = prompt.find("{user}")) != std::string::npos)
    prompt.replace(pos, 6, username); // Replace {user} with username

  return prompt;
}

void setRawMode() {
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag &=
      ~(ICANON | ECHO); // Disable canonical mode (buffered input) and echo
  term.c_cc[VMIN] = 1;  // Minimum number of characters to read
  term.c_cc[VTIME] = 0; // No timeout
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void resetTerminal() {
  termios term;
  tcgetattr(STDIN_FILENO, &term);
  term.c_lflag |=
      (ICANON | ECHO); // Enable canonical mode (buffered input) and echo
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

// Array of built-in commands
array<string, 9> BuiltIns = {"cd", "ls", "echo", "pwd"};

void HandleCommands(string command) {
  // Check if the command is a built-in command
  for (int i = 0; i < BuiltIns.size(); i++) {
    if (command == BuiltIns[i]) {
      string cmd = command;
      istringstream stream(cmd);
      string arg;
      vector<string> args;

      // Split the command into words (arguments)
      while (stream >> arg) {
        args.push_back(arg);
      }

      // You can now switch based on args[0] or use if statements to handle
      // specific commands
      if (args[0] == "cd") {

        if (chdir(args[1].c_str()) != 0) {
          perror("cd failed");
        }
        // Handle cd logic here
      } else if (args[0] == "echo") {

        for (size_t i = 1; i < args.size(); ++i) {
          cout << args[i] << " ";
        }
        cout << endl;
      } else if (args[0] == "pwd") {
        string CurrentWorkingDirectory =
            getcwd(NULL, 0); // Get current working directory
        cout << "\n" << CurrentWorkingDirectory << endl;
      } else if (args[0] == "ls") {
      }
      // Add more built-in command cases as needed

      return; // Exit the function after handling the built-in command
    }
  }

  // If not a built-in command, execute it using system()

  system(command.c_str());
}

std::string readInput() {
  std::string input;
  char ch;
  // Reset colors before input
  std::cout << "\033[0m";
  while (true) {
    ch = getchar();
    if (ch == '\n') {
      std::cout << "\033[0m" << std::endl; // Reset colors and newline
      break;
    }
    if (ch == 127) { // Backspace
      if (!input.empty()) {
        input.pop_back();
        std::cout << "\b \b";
      }
    } else {
      input.push_back(ch);
      std::cout << ch;
    }
  }
  return input;
}

int main() {
  string configFilePath = "config.yaml"; // Path to your config file
  if (!createConfigFile(configFilePath)) {
    std::cout << "Using existing config file." << std::endl;
  }

  // Initialize ConfigManager here
  ConfigManager configManager(configFilePath);

  // Get the custom prompt, background color, and text color from the config
  std::string customPrompt = configManager.getPrompt();
  std::string bgColor = configManager.getBgColor();
  std::string textColor = configManager.getTextColor();

  // Replace placeholders in the prompt
  customPrompt = ReplacePlaceholders(customPrompt); // <-- Add this line

  // Display the settings (for testing purposes)
  std::cout << "Prompt: " << customPrompt << std::endl;
  std::cout << "Background color: " << bgColor << std::endl;
  std::cout << "Text color: " << textColor << std::endl;

  // Your main shell loop
  while (true) {
    std::cout
        << configManager.getFormattedPrompt(); // Display the custom prompt

    // Read user input
    std::string input = readInput();

    // Handle exit command
    if (input == "exit") {
      break; // Exit the shell
    }

    // Handle the command
    HandleCommands(input);
  }

  resetTerminal(); // Make sure to reset the terminal before exiting
  return 0;
}

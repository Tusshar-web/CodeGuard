#ifndef CLI_PARSER_H
#define CLI_PARSER_H

#include <string>
#include <vector>

struct CliOptions {
    std::string targetPath;
    std::string format = "text";      // "text" or "json"
    std::string severityFilter = "";  // "HIGH", "MEDIUM", "LOW", or empty (all)
    std::string outputFile = "";      // path to save report, empty = stdout
    bool showHelp = false;
    bool showVersion = false;
    bool valid = true;
    std::string errorMessage;
};

class CliParser {
public:
    static CliOptions parse(int argc, char* argv[]);
    static void printHelp();
    static void printVersion();
};

#endif


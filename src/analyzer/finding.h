#ifndef FINDING_H
#define FINDING_H

#include <string>
#include <utility>

struct Finding {
    std::string filePath;
    int lineNumber = 0;
    std::string ruleName;       // e.g. "STR001"
    std::string severity;       // "HIGH", "MEDIUM", "LOW"
    std::string message;        // Description of vulnerability
    std::string functionName;   // e.g. "copyData" or "main"
    std::string remediation;    // Recommended remediation
    std::string sourceLocation; // Source location for data-flow findings (e.g. "file.cpp:12")
    std::string sinkLocation;   // Sink location for data-flow findings (e.g. "file.cpp:18")

    Finding() = default;

    Finding(std::string file,
            int line,
            std::string rule,
            std::string sev,
            std::string msg,
            std::string func = "",
            std::string remed = "",
            std::string srcLoc = "",
            std::string snkLoc = "")
        : filePath(std::move(file)),
          lineNumber(line),
          ruleName(std::move(rule)),
          severity(std::move(sev)),
          message(std::move(msg)),
          functionName(std::move(func)),
          remediation(std::move(remed)),
          sourceLocation(std::move(srcLoc)),
          sinkLocation(std::move(snkLoc)) {}
};

#endif

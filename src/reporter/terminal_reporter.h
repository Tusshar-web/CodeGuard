#ifndef TERMINAL_REPORTER_H
#define TERMINAL_REPORTER_H

#include "reporter.h"

class TerminalReporter : public Reporter {
public:
    void generateReport(const std::vector<Finding>& findings,
                        const ScanStatistics& stats,
                        std::ostream& out) override;

    std::string renderToString(const std::vector<Finding>& findings,
                               const ScanStatistics& stats) override;

private:
    static std::string getRuleTitle(const std::string& ruleId);
};

#endif


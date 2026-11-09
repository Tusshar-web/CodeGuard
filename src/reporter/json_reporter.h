#ifndef JSON_REPORTER_H
#define JSON_REPORTER_H

#include "reporter.h"

class JsonReporter : public Reporter {
public:
    void generateReport(const std::vector<Finding>& findings,
                        const ScanStatistics& stats,
                        std::ostream& out) override;

    std::string renderToString(const std::vector<Finding>& findings,
                               const ScanStatistics& stats) override;

private:
    static std::string escapeJson(const std::string& input);
};

#endif


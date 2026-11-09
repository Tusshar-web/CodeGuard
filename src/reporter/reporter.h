#ifndef REPORTER_H
#define REPORTER_H

#include <vector>
#include <string>
#include <ostream>
#include "../analyzer/finding.h"

struct ScanStatistics {
    size_t filesDiscovered = 0;
    size_t filesAnalyzed = 0;
    size_t failedAnalyses = 0;
    size_t totalFindings = 0;
    size_t highSeverity = 0;
    size_t mediumSeverity = 0;
    size_t lowSeverity = 0;

    static ScanStatistics compute(size_t discovered,
                                  size_t analyzed,
                                  size_t failed,
                                  const std::vector<Finding>& findings) {
        ScanStatistics stats;
        stats.filesDiscovered = discovered;
        stats.filesAnalyzed = analyzed;
        stats.failedAnalyses = failed;
        stats.totalFindings = findings.size();
        for (const auto& f : findings) {
            if (f.severity == "HIGH") {
                stats.highSeverity++;
            } else if (f.severity == "MEDIUM") {
                stats.mediumSeverity++;
            } else if (f.severity == "LOW") {
                stats.lowSeverity++;
            }
        }
        return stats;
    }
};

class Reporter {
public:
    virtual ~Reporter() = default;
    virtual void generateReport(const std::vector<Finding>& findings,
                                const ScanStatistics& stats,
                                std::ostream& out) = 0;
    virtual std::string renderToString(const std::vector<Finding>& findings,
                                       const ScanStatistics& stats) = 0;
};

#endif


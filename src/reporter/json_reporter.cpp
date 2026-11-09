#include "json_reporter.h"
#include <sstream>

std::string JsonReporter::escapeJson(const std::string& input) {
    std::string output;
    output.reserve(input.size() + 16);
    for (char c : input) {
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b";  break;
            case '\f': output += "\\f";  break;
            case '\n': output += "\\n";  break;
            case '\r': output += "\\r";  break;
            case '\t': output += "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    output += buf;
                } else {
                    output += c;
                }
                break;
        }
    }
    return output;
}

void JsonReporter::generateReport(const std::vector<Finding>& findings,
                                  const ScanStatistics& stats,
                                  std::ostream& out) {
    out << "{\n";
    out << "  \"tool\": \"CodeGuard\",\n";
    out << "  \"version\": \"1.0\",\n";
    out << "  \"files_scanned\": " << stats.filesDiscovered << ",\n";
    out << "  \"files_analyzed\": " << stats.filesAnalyzed << ",\n";
    out << "  \"failed_analyses\": " << stats.failedAnalyses << ",\n";
    out << "  \"summary\": {\n";
    out << "    \"total_findings\": " << stats.totalFindings << ",\n";
    out << "    \"high\": " << stats.highSeverity << ",\n";
    out << "    \"medium\": " << stats.mediumSeverity << ",\n";
    out << "    \"low\": " << stats.lowSeverity << "\n";
    out << "  },\n";
    out << "  \"findings\": [";

    for (size_t i = 0; i < findings.size(); ++i) {
        const auto& f = findings[i];
        out << "\n    {\n";
        out << "      \"rule_id\": \"" << escapeJson(f.ruleName) << "\",\n";
        out << "      \"severity\": \"" << escapeJson(f.severity) << "\",\n";
        out << "      \"file\": \"" << escapeJson(f.filePath) << "\",\n";
        out << "      \"line\": " << f.lineNumber << ",\n";
        out << "      \"function\": \"" << escapeJson(f.functionName.empty() ? "" : f.functionName + "()") << "\",\n";
        out << "      \"message\": \"" << escapeJson(f.message) << "\",\n";
        out << "      \"remediation\": \"" << escapeJson(f.remediation) << "\"";
        if (!f.sourceLocation.empty()) {
            out << ",\n      \"source\": \"" << escapeJson(f.sourceLocation) << "\"";
        }
        if (!f.sinkLocation.empty()) {
            out << ",\n      \"sink\": \"" << escapeJson(f.sinkLocation) << "\"";
        }
        out << "\n    }";
        if (i + 1 < findings.size()) {
            out << ",";
        }
    }

    if (!findings.empty()) {
        out << "\n  ";
    }
    out << "]\n";
    out << "}\n";
}

std::string JsonReporter::renderToString(const std::vector<Finding>& findings,
                                        const ScanStatistics& stats) {
    std::ostringstream ss;
    generateReport(findings, stats, ss);
    return ss.str();
}


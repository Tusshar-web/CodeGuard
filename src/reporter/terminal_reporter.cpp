#include "terminal_reporter.h"
#include <iomanip>
#include <sstream>

std::string TerminalReporter::getRuleTitle(const std::string& ruleId) {
    if (ruleId == "STR001") return "Buffer Overflow (strcpy)";
    if (ruleId == "STR002") return "Dangerous Function (gets)";
    if (ruleId == "STR003") return "Buffer Overflow (sprintf)";
    if (ruleId == "STR004") return "Unterminated String (strncpy)";
    if (ruleId == "MEM001") return "Buffer Overflow (memcpy)";
    if (ruleId == "MEM002") return "Suspicious Memory Operation (memmove)";
    if (ruleId == "MEM003") return "Potential Use-After-Free";
    if (ruleId == "INT001") return "Risky Integer Arithmetic";
    if (ruleId == "CMD001") return "Potential Command Injection";
    if (ruleId == "PTR001") return "Potential Null Pointer Dereference";
    return "Security Finding";
}

void TerminalReporter::generateReport(const std::vector<Finding>& findings,
                                      const ScanStatistics& stats,
                                      std::ostream& out) {
    out << "========================================\n";
    out << "          CODEGUARD SECURITY REPORT\n";
    out << "========================================\n";

    for (size_t i = 0; i < findings.size(); ++i) {
        const auto& f = findings[i];
        out << "\n[" << f.severity << "] " << f.ruleName << " - " << getRuleTitle(f.ruleName) << "\n\n";
        out << "File: " << f.filePath << "\n";
        out << "Line: " << f.lineNumber << "\n";
        if (!f.functionName.empty()) {
            out << "Function: " << f.functionName << "()\n";
        }
        if (!f.sourceLocation.empty()) {
            out << "Source: " << f.sourceLocation << "\n";
        }
        if (!f.sinkLocation.empty()) {
            out << "Sink: " << f.sinkLocation << "\n";
        }
        out << "\nDescription:\n" << f.message << "\n\n";
        if (!f.remediation.empty()) {
            out << "Recommendation:\n" << f.remediation << "\n";
        }
        out << "\n----------------------------------------\n";
    }

    if (findings.empty()) {
        out << "\nNo vulnerabilities detected.\n";
        out << "\n----------------------------------------\n";
    }

    out << "\nSUMMARY\n\n";
    out << std::left << std::setw(24) << "Files discovered:" << stats.filesDiscovered << "\n";
    out << std::left << std::setw(24) << "Files analyzed:" << stats.filesAnalyzed << "\n";
    out << std::left << std::setw(24) << "Failed analyses:" << stats.failedAnalyses << "\n";
    out << std::left << std::setw(24) << "Total findings:" << stats.totalFindings << "\n\n";
    out << std::left << std::setw(24) << "HIGH:" << stats.highSeverity << "\n";
    out << std::left << std::setw(24) << "MEDIUM:" << stats.mediumSeverity << "\n";
    out << std::left << std::setw(24) << "LOW:" << stats.lowSeverity << "\n\n";
    out << "========================================\n";
}

std::string TerminalReporter::renderToString(const std::vector<Finding>& findings,
                                            const ScanStatistics& stats) {
    std::ostringstream ss;
    generateReport(findings, stats, ss);
    return ss.str();
}


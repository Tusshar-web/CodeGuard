#include "ast_analyzer.h"

#include <string>
#include <vector>
#include <set>
#include <tuple>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <memory>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Frontend/FrontendActions.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Tooling/CompilationDatabase.h>

using namespace std;
using namespace clang;
using namespace clang::tooling;

namespace {

// Helper: Get capacity of fixed-size array in bytes
long long getBufferCapacity(Expr *expr, ASTContext *context) {
    if (!expr) return -1;
    expr = expr->IgnoreParenImpCasts();

    if (auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
        if (auto *var = dyn_cast<VarDecl>(declRef->getDecl())) {
            if (const ConstantArrayType *arrType = context->getAsConstantArrayType(var->getType())) {
                return static_cast<long long>(arrType->getSize().getZExtValue());
            }
            if (var->hasInit()) {
                Expr *init = var->getInit()->IgnoreParenImpCasts();
                if (auto *lit = dyn_cast<StringLiteral>(init)) {
                    return static_cast<long long>(lit->getLength() + 1);
                }
            }
        }
    }
    return -1;
}

// Helper: Get length of string literal or string-initialized variable (+1 for null terminator)
long long getSourceStringLength(Expr *expr) {
    if (!expr) return -1;
    expr = expr->IgnoreParenImpCasts();

    if (auto *lit = dyn_cast<StringLiteral>(expr)) {
        return static_cast<long long>(lit->getLength() + 1);
    }

    if (auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
        if (auto *var = dyn_cast<VarDecl>(declRef->getDecl())) {
            if (var->hasInit()) {
                Expr *init = var->getInit()->IgnoreParenImpCasts();
                if (auto *lit = dyn_cast<StringLiteral>(init)) {
                    return static_cast<long long>(lit->getLength() + 1);
                }
            }
        }
    }
    return -1;
}

// Helper: Extract variable name from an expression
std::string extractVariableName(Expr *expr) {
    if (!expr) return "";
    expr = expr->IgnoreParenImpCasts();

    if (auto *declRef = dyn_cast<DeclRefExpr>(expr)) {
        return declRef->getNameInfo().getAsString();
    }
    if (auto *arraySub = dyn_cast<ArraySubscriptExpr>(expr)) {
        return extractVariableName(arraySub->getBase());
    }
    if (auto *unary = dyn_cast<UnaryOperator>(expr)) {
        return extractVariableName(unary->getSubExpr());
    }
    return "";
}

// Helper: Check if an expression represents a non-constant arithmetic operation (e.g. n * sizeof(int))
bool isRiskyArithmetic(Expr *expr, ASTContext *context) {
    if (!expr) return false;
    expr = expr->IgnoreParenImpCasts();

    if (auto *bin = dyn_cast<BinaryOperator>(expr)) {
        if (bin->getOpcode() == BO_Mul || bin->getOpcode() == BO_Add) {
            // If the expression can be evaluated as a compile-time integer constant, it's safe
            Expr::EvalResult result;
            if (expr->EvaluateAsInt(result, *context)) {
                return false;
            }
            // Otherwise, it involves variable arithmetic in size calculation
            return true;
        }
    }
    return false;
}

} // namespace

class SecurityVisitor : public RecursiveASTVisitor<SecurityVisitor> {
private:
    ASTContext *context;
    string filePath;
    vector<Finding> &findings;
    CallGraph &callGraph;
    DataFlowTracker &dataFlow;
    FunctionDecl *currentFunction = nullptr;
    set<tuple<string, int, string>> seenFindings;

    void addFinding(const Finding &f) {
        auto key = make_tuple(f.filePath, f.lineNumber, f.ruleName);
        if (seenFindings.find(key) == seenFindings.end()) {
            seenFindings.insert(key);
            findings.push_back(f);
        }
    }

    string getCurrentFunctionName() const {
        return currentFunction ? currentFunction->getNameAsString() : "";
    }

    bool isCallSafeStrcpy(CallExpr *call) {
        if (call->getNumArgs() < 2) return false;
        long long destCapacity = getBufferCapacity(call->getArg(0), context);
        long long srcLength = getSourceStringLength(call->getArg(1));
        if (destCapacity > 0 && srcLength > 0) {
            return srcLength <= destCapacity;
        }
        return false;
    }

    bool isCallSafeSprintf(CallExpr *call) {
        if (call->getNumArgs() < 2) return false;
        long long destCapacity = getBufferCapacity(call->getArg(0), context);
        if (destCapacity <= 0) return false;

        Expr *formatExpr = call->getArg(1)->IgnoreParenImpCasts();
        auto *formatLit = dyn_cast<StringLiteral>(formatExpr);
        if (!formatLit) return false;

        string format = formatLit->getString().str();
        if (format == "%s" && call->getNumArgs() == 3) {
            long long srcLength = getSourceStringLength(call->getArg(2));
            if (srcLength > 0 && srcLength <= destCapacity) {
                return true;
            }
        }
        return false;
    }

    bool isCallSafeStrncpy(CallExpr *call) {
        if (call->getNumArgs() < 3) return false;
        long long destCapacity = getBufferCapacity(call->getArg(0), context);
        if (destCapacity <= 0) return false;

        Expr *countExpr = call->getArg(2)->IgnoreParenImpCasts();
        auto *countLit = dyn_cast<IntegerLiteral>(countExpr);
        if (!countLit) return false;

        unsigned long long copySize = countLit->getValue().getZExtValue();
        return copySize < static_cast<unsigned long long>(destCapacity);
    }

    bool isCallSafeMemcpy(CallExpr *call) {
        if (call->getNumArgs() < 3) return false;
        long long destCapacity = getBufferCapacity(call->getArg(0), context);
        if (destCapacity <= 0) return false;

        Expr *countExpr = call->getArg(2)->IgnoreParenImpCasts();
        auto *countLit = dyn_cast<IntegerLiteral>(countExpr);
        if (!countLit) return false;

        unsigned long long copySize = countLit->getValue().getZExtValue();
        return copySize <= static_cast<unsigned long long>(destCapacity);
    }

    bool isCallSafeMemmove(CallExpr *call) {
        if (call->getNumArgs() < 3) return false;
        long long destCapacity = getBufferCapacity(call->getArg(0), context);
        long long srcCapacity = getBufferCapacity(call->getArg(1), context);

        Expr *countExpr = call->getArg(2)->IgnoreParenImpCasts();
        auto *countLit = dyn_cast<IntegerLiteral>(countExpr);
        if (!countLit) return false;

        unsigned long long copySize = countLit->getValue().getZExtValue();
        bool safeDest = (destCapacity <= 0) || (copySize <= static_cast<unsigned long long>(destCapacity));
        bool safeSrc = (srcCapacity <= 0) || (copySize <= static_cast<unsigned long long>(srcCapacity));
        return safeDest && safeSrc && (destCapacity > 0 || srcCapacity > 0);
    }

    void inspectUseAfterFreeAndNullDeref(FunctionDecl *FD) {
        if (!FD || !FD->hasBody()) return;
        Stmt *body = FD->getBody();
        if (!body) return;

        SourceManager &sm = context->getSourceManager();

        // Pass 1: Find pointer deallocations (free/delete) and null initializations
        struct VarStatus {
            const VarDecl *var = nullptr;
            int freeLine = 0;
            int nullInitLine = 0;
            bool isFreed = false;
            bool isNull = false;
        };

        map<const VarDecl*, VarStatus> varStatuses;

        // Collect pointers initialized to nullptr / NULL / 0
        for (const Decl *d : FD->decls()) {
            if (const auto *vd = dyn_cast<VarDecl>(d)) {
                if (vd->getType()->isPointerType() && vd->hasInit()) {
                    const Expr *init = vd->getInit()->IgnoreParenImpCasts();
                    if (init->isNullPointerConstant(*context, Expr::NPC_ValueDependentIsNotNull)) {
                        SourceLocation loc = sm.getExpansionLoc(vd->getLocation());
                        int line = sm.getSpellingLineNumber(loc);
                        varStatuses[vd] = {vd, 0, line, false, true};
                    }
                }
            }
        }

        // Helper visitor to scan statements inside the body in order
        class BodyVisitor : public RecursiveASTVisitor<BodyVisitor> {
        public:
            ASTContext *ctx;
            SourceManager &sm;
            map<const VarDecl*, VarStatus> &statuses;
            SecurityVisitor &parent;

            BodyVisitor(ASTContext *ctx, SourceManager &sm, map<const VarDecl*, VarStatus> &st, SecurityVisitor &p)
                : ctx(ctx), sm(sm), statuses(st), parent(p) {}

            // Check if variable is reassigned: ptr = ...
            bool VisitBinaryOperator(BinaryOperator *bin) {
                if (bin->getOpcode() == BO_Assign) {
                    Expr *lhs = bin->getLHS()->IgnoreParenImpCasts();
                    if (auto *declRef = dyn_cast<DeclRefExpr>(lhs)) {
                        if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                            auto it = statuses.find(vd);
                            if (it != statuses.end()) {
                                Expr *rhs = bin->getRHS()->IgnoreParenImpCasts();
                                if (!rhs->isNullPointerConstant(*ctx, Expr::NPC_ValueDependentIsNotNull)) {
                                    it->second.isFreed = false;
                                    it->second.isNull = false;
                                }
                            }
                        }
                    }
                }
                return true;
            }

            // Check free(p) or delete p
            bool VisitCallExpr(CallExpr *call) {
                FunctionDecl *callee = call->getDirectCallee();
                if (callee && callee->getNameAsString() == "free" && call->getNumArgs() >= 1) {
                    Expr *arg = call->getArg(0)->IgnoreParenImpCasts();
                    if (auto *declRef = dyn_cast<DeclRefExpr>(arg)) {
                        if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                            SourceLocation loc = sm.getExpansionLoc(call->getExprLoc());
                            int line = sm.getSpellingLineNumber(loc);
                            statuses[vd].var = vd;
                            statuses[vd].isFreed = true;
                            statuses[vd].freeLine = line;
                        }
                    }
                }
                return true;
            }

            bool VisitCXXDeleteExpr(CXXDeleteExpr *delExpr) {
                Expr *arg = delExpr->getArgument()->IgnoreParenImpCasts();
                if (auto *declRef = dyn_cast<DeclRefExpr>(arg)) {
                    if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                        SourceLocation loc = sm.getExpansionLoc(delExpr->getExprLoc());
                        int line = sm.getSpellingLineNumber(loc);
                        statuses[vd].var = vd;
                        statuses[vd].isFreed = true;
                        statuses[vd].freeLine = line;
                    }
                }
                return true;
            }

            // Check dereference: *p, p->foo, p[i]
            bool VisitUnaryOperator(UnaryOperator *unary) {
                if (unary->getOpcode() == UO_Deref) {
                    Expr *sub = unary->getSubExpr()->IgnoreParenImpCasts();
                    if (auto *declRef = dyn_cast<DeclRefExpr>(sub)) {
                        if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                            checkDeref(vd, unary->getExprLoc());
                        }
                    }
                }
                return true;
            }

            bool VisitMemberExpr(MemberExpr *mem) {
                if (mem->isArrow()) {
                    Expr *base = mem->getBase()->IgnoreParenImpCasts();
                    if (auto *declRef = dyn_cast<DeclRefExpr>(base)) {
                        if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                            checkDeref(vd, mem->getExprLoc());
                        }
                    }
                }
                return true;
            }

            bool VisitArraySubscriptExpr(ArraySubscriptExpr *arr) {
                Expr *base = arr->getBase()->IgnoreParenImpCasts();
                if (auto *declRef = dyn_cast<DeclRefExpr>(base)) {
                    if (auto *vd = dyn_cast<VarDecl>(declRef->getDecl())) {
                        checkDeref(vd, arr->getExprLoc());
                    }
                }
                return true;
            }

        private:
            void checkDeref(const VarDecl *vd, SourceLocation loc) {
                loc = sm.getExpansionLoc(loc);
                if (sm.isInSystemHeader(loc) || !sm.isWrittenInMainFile(loc)) return;
                int line = sm.getSpellingLineNumber(loc);

                auto it = statuses.find(vd);
                if (it != statuses.end()) {
                    if (it->second.isFreed && line > it->second.freeLine) {
                        parent.addFinding({
                            parent.filePath,
                            line,
                            "MEM003",
                            "HIGH",
                            "Potential use-after-free: pointer '" + vd->getNameAsString() +
                            "' is dereferenced after being freed at line " + to_string(it->second.freeLine),
                            parent.getCurrentFunctionName(),
                            "Do not access memory after deallocation; set pointer to nullptr immediately after freeing."
                        });
                    }
                    if (it->second.isNull && line > it->second.nullInitLine) {
                        parent.addFinding({
                            parent.filePath,
                            line,
                            "PTR001",
                            "HIGH",
                            "Potential null pointer dereference: pointer '" + vd->getNameAsString() +
                            "' was initialized to null at line " + to_string(it->second.nullInitLine) +
                            " and dereferenced without reassignment or null-check.",
                            parent.getCurrentFunctionName(),
                            "Validate that pointer is non-null before dereferencing, or initialize with valid memory address."
                        });
                    }
                }
            }
        };

        BodyVisitor bodyVisitor(context, sm, varStatuses, *this);
        bodyVisitor.TraverseStmt(body);
    }

public:
    SecurityVisitor(ASTContext *context,
                    const string &filePath,
                    vector<Finding> &findings,
                    CallGraph &callGraph,
                    DataFlowTracker &dataFlow)
        : context(context),
          filePath(filePath),
          findings(findings),
          callGraph(callGraph),
          dataFlow(dataFlow) {}

    bool TraverseFunctionDecl(FunctionDecl *FD) {
        if (!FD) return true;

        SourceManager &sm = context->getSourceManager();
        SourceLocation loc = sm.getExpansionLoc(FD->getLocation());
        if (sm.isInSystemHeader(loc)) return true;

        FunctionDecl *prevFunction = currentFunction;
        currentFunction = FD;

        // Record function summary for CallGraph
        FunctionSummary summary;
        summary.name = FD->getNameAsString();
        summary.returnType = FD->getReturnType().getAsString();
        summary.isDefinition = FD->hasBody();
        summary.filePath = filePath;
        summary.definitionLine = sm.getSpellingLineNumber(loc);

        for (unsigned i = 0; i < FD->getNumParams(); ++i) {
            ParmVarDecl *param = FD->getParamDecl(i);
            summary.parameters.push_back({
                param->getNameAsString(),
                param->getType().getAsString(),
                static_cast<int>(i)
            });
        }

        // If main(int argc, char* argv[]), mark argv as untrusted input source
        if (FD->isMain() && FD->getNumParams() >= 2) {
            dataFlow.markTainted(FD->getParamDecl(1)->getNameAsString(), "argv", filePath, summary.definitionLine);
        }

        // Traverse the function's AST
        bool result = RecursiveASTVisitor<SecurityVisitor>::TraverseFunctionDecl(FD);

        // Analyze use-after-free and null pointer dereferences inside the function body
        if (FD->hasBody() && sm.isWrittenInMainFile(loc)) {
            inspectUseAfterFreeAndNullDeref(FD);
        }

        callGraph.addFunctionSummary(summary);
        currentFunction = prevFunction;
        return result;
    }

    bool VisitCallExpr(CallExpr *call) {
        FunctionDecl *callee = call->getDirectCallee();
        if (!callee) return true;

        SourceManager &sm = context->getSourceManager();
        SourceLocation loc = sm.getExpansionLoc(call->getExprLoc());
        if (sm.isInSystemHeader(loc) || !sm.isWrittenInMainFile(loc)) return true;

        int line = sm.getSpellingLineNumber(loc);
        string name = callee->getNameAsString();
        string callerName = getCurrentFunctionName();

        // Record call in CallGraph
        CallSite callSite;
        callSite.callerName = callerName;
        callSite.calleeName = name;
        callSite.filePath = filePath;
        callSite.lineNumber = line;

        for (unsigned i = 0; i < call->getNumArgs(); ++i) {
            Expr *arg = call->getArg(i)->IgnoreParenImpCasts();
            long long cap = getBufferCapacity(arg, context);
            long long strLen = getSourceStringLength(arg);
            long long knownSize = (cap > 0) ? cap : strLen;
            callSite.knownArgSizes.push_back(knownSize);
            bool isLit = isa<StringLiteral>(arg);
            callSite.isStringLiteral.push_back(isLit);
            if (isLit) {
                callSite.stringLiteralValues.push_back(cast<StringLiteral>(arg)->getString().str());
            } else {
                callSite.stringLiteralValues.push_back("");
            }
        }
        callGraph.addCallSite(callSite);

        // Check if current function is copying between parameters (for interprocedural analysis)
        if (currentFunction && currentFunction->hasBody() && (name == "strcpy" || name == "memcpy")) {
            if (call->getNumArgs() >= 2) {
                string destVar = extractVariableName(call->getArg(0));
                string srcVar = extractVariableName(call->getArg(1));

                int destIdx = -1;
                int srcIdx = -1;
                for (unsigned i = 0; i < currentFunction->getNumParams(); ++i) {
                    if (currentFunction->getParamDecl(i)->getNameAsString() == destVar) destIdx = i;
                    if (currentFunction->getParamDecl(i)->getNameAsString() == srcVar) srcIdx = i;
                }

                if (destIdx >= 0 && srcIdx >= 0) {
                    // This function copies param(srcIdx) to param(destIdx)
                    FunctionSummary summary;
                    summary.name = currentFunction->getNameAsString();
                    summary.returnType = currentFunction->getReturnType().getAsString();
                    summary.isDefinition = true;
                    summary.filePath = filePath;
                    summary.definitionLine = sm.getSpellingLineNumber(sm.getExpansionLoc(currentFunction->getLocation()));
                    ParameterCopyOp op;
                    op.destParamIndex = destIdx;
                    op.srcParamIndex = srcIdx;
                    op.opName = name;
                    op.line = line;
                    summary.paramCopyOps.push_back(op);
                    callGraph.addFunctionSummary(summary);
                }
            }
        }

        // STR001: strcpy
        if (name == "strcpy") {
            if (call->getNumArgs() >= 2) {
                string srcVar = extractVariableName(call->getArg(1));
                if (!srcVar.empty() && dataFlow.isTainted(srcVar)) {
                    dataFlow.recordSink(srcVar, "strcpy", filePath, line, callerName);
                }
            }

            if (!isCallSafeStrcpy(call)) {
                // If destination capacity is explicitly known and smaller than source:
                long long destCap = (call->getNumArgs() >= 1) ? getBufferCapacity(call->getArg(0), context) : -1;
                long long srcLen = (call->getNumArgs() >= 2) ? getSourceStringLength(call->getArg(1)) : -1;

                string msg = "Potential buffer overflow: strcpy() source may exceed destination capacity";
                if (destCap > 0 && srcLen > 0 && srcLen > destCap) {
                    msg = "Potential buffer overflow: destination capacity is " + to_string(destCap) +
                          " bytes, but source string length is " + to_string(srcLen) + " bytes.";
                }

                addFinding({
                    filePath,
                    line,
                    "STR001",
                    "HIGH",
                    msg,
                    callerName,
                    "Use bounded operations such as strncpy(), strlcpy(), or std::string, and validate bounds."
                });
            }
        }
        // STR002: gets
        else if (name == "gets") {
            addFinding({
                filePath,
                line,
                "STR002",
                "HIGH",
                "Unsafe use of gets(): gets() does not perform bounds checking and is inherently vulnerable.",
                callerName,
                "Replace gets() with fgets() or std::string / std::getline() to prevent buffer overflows."
            });
        }
        // STR003: sprintf
        else if (name == "sprintf") {
            if (call->getNumArgs() >= 3) {
                for (unsigned i = 2; i < call->getNumArgs(); ++i) {
                    string var = extractVariableName(call->getArg(i));
                    if (!var.empty() && dataFlow.isTainted(var)) {
                        dataFlow.recordSink(var, "sprintf", filePath, line, callerName);
                    }
                }
            }

            if (!isCallSafeSprintf(call)) {
                addFinding({
                    filePath,
                    line,
                    "STR003",
                    "HIGH",
                    "Potential buffer overflow: sprintf() output may exceed destination capacity.",
                    callerName,
                    "Use snprintf() specifying the destination buffer capacity to prevent overflows."
                });
            }
        }
        // STR004: strncpy
        else if (name == "strncpy") {
            if (!isCallSafeStrncpy(call)) {
                addFinding({
                    filePath,
                    line,
                    "STR004",
                    "HIGH",
                    "Potential missing null terminator: strncpy() may leave destination unterminated when copy length reaches buffer size.",
                    callerName,
                    "Explicitly null-terminate destination buffer: buffer[sizeof(buffer) - 1] = '\\0', or use std::string."
                });
            }
        }
        // MEM001: memcpy
        else if (name == "memcpy") {
            if (call->getNumArgs() >= 3) {
                string srcVar = extractVariableName(call->getArg(1));
                string countVar = extractVariableName(call->getArg(2));
                if (!srcVar.empty() && dataFlow.isTainted(srcVar)) {
                    dataFlow.recordSink(srcVar, "memcpy", filePath, line, callerName);
                }
                if (!countVar.empty() && dataFlow.isTainted(countVar)) {
                    dataFlow.recordSink(countVar, "memcpy", filePath, line, callerName);
                }
            }

            if (!isCallSafeMemcpy(call)) {
                long long destCap = (call->getNumArgs() >= 1) ? getBufferCapacity(call->getArg(0), context) : -1;
                long long copySize = -1;
                if (call->getNumArgs() >= 3) {
                    if (auto *cntLit = dyn_cast<IntegerLiteral>(call->getArg(2)->IgnoreParenImpCasts())) {
                        copySize = cntLit->getValue().getZExtValue();
                    }
                }

                string msg = "Potential buffer overflow: memcpy() copy size exceeds destination capacity.";
                if (destCap > 0 && copySize > destCap) {
                    msg = "Potential buffer overflow: memcpy() copies " + to_string(copySize) +
                          " bytes into destination buffer of capacity " + to_string(destCap) + " bytes.";
                }

                addFinding({
                    filePath,
                    line,
                    "MEM001",
                    "HIGH",
                    msg,
                    callerName,
                    "Ensure copy size does not exceed destination buffer size, or validate bounds before copying."
                });
            }
        }
        // MEM002: memmove
        else if (name == "memmove") {
            if (!isCallSafeMemmove(call)) {
                addFinding({
                    filePath,
                    line,
                    "MEM002",
                    "MEDIUM",
                    "Suspicious memory operation: memmove() byte count exceeds destination or source buffer capacity.",
                    callerName,
                    "Validate that byte count does not exceed either source or destination buffer bounds."
                });
            }
        }
        // CMD001: system or popen
        else if (name == "system" || name == "popen") {
            if (call->getNumArgs() >= 1) {
                Expr *cmdArg = call->getArg(0)->IgnoreParenImpCasts();
                string cmdVar = extractVariableName(cmdArg);

                if (!cmdVar.empty() && dataFlow.isTainted(cmdVar)) {
                    dataFlow.recordSink(cmdVar, name, filePath, line, callerName);
                } else if (!isa<StringLiteral>(cmdArg)) {
                    addFinding({
                        filePath,
                        line,
                        "CMD001",
                        "HIGH",
                        "Potential command injection: non-literal or dynamic command passed to " + name + "().",
                        callerName,
                        "Avoid passing dynamic or unverified commands to system(); use execve() with structured arguments."
                    });
                }
            }
        }
        // INT001: malloc / calloc integer overflow
        else if (name == "malloc") {
            if (call->getNumArgs() >= 1) {
                Expr *sizeArg = call->getArg(0);
                if (isRiskyArithmetic(sizeArg, context)) {
                    addFinding({
                        filePath,
                        line,
                        "INT001",
                        "MEDIUM",
                        "Potential integer overflow: arithmetic expression in malloc() size without overflow validation.",
                        callerName,
                        "Validate arithmetic operands or check for overflow before memory allocation."
                    });
                }
            }
        }
        // Input sources: scanf, fgets, read, getenv
        else if (name == "scanf" || name == "__isoc99_scanf") {
            for (unsigned i = 1; i < call->getNumArgs(); ++i) {
                string var = extractVariableName(call->getArg(i));
                if (!var.empty()) {
                    dataFlow.markTainted(var, "scanf", filePath, line);
                }
            }
        }
        else if (name == "fgets") {
            if (call->getNumArgs() >= 1) {
                string var = extractVariableName(call->getArg(0));
                if (!var.empty()) {
                    dataFlow.markTainted(var, "fgets", filePath, line);
                }
            }
        }
        else if (name == "getenv") {
            // Returned string is untrusted environment data
        }

        return true;
    }

    // Check operator>> (e.g. cin >> input)
    bool VisitCXXOperatorCallExpr(CXXOperatorCallExpr *opCall) {
        if (opCall->getOperator() == OO_GreaterGreater && opCall->getNumArgs() >= 2) {
            Expr *lhs = opCall->getArg(0)->IgnoreParenImpCasts();
            string lhsName = extractVariableName(lhs);

            if (lhsName == "cin" || lhsName == "std::cin") {
                Expr *rhs = opCall->getArg(1)->IgnoreParenImpCasts();
                string rhsVar = extractVariableName(rhs);
                if (!rhsVar.empty()) {
                    SourceManager &sm = context->getSourceManager();
                    SourceLocation loc = sm.getExpansionLoc(opCall->getExprLoc());
                    int line = sm.getSpellingLineNumber(loc);
                    dataFlow.markTainted(rhsVar, "cin", filePath, line);
                }
            }
        }
        return true;
    }

    // Check variable assignments: data = input
    bool VisitBinaryOperator(BinaryOperator *binOp) {
        if (binOp->getOpcode() == BO_Assign) {
            string dest = extractVariableName(binOp->getLHS());
            string src = extractVariableName(binOp->getRHS());
            if (!dest.empty() && !src.empty() && dataFlow.isTainted(src)) {
                dataFlow.propagateTaint(dest, src);
            }
        }
        return true;
    }

    // Check variable declaration initializations: char* data = input;
    bool VisitVarDecl(VarDecl *var) {
        if (var->hasInit()) {
            string dest = var->getNameAsString();
            string src = extractVariableName(var->getInit());
            if (!dest.empty() && !src.empty() && dataFlow.isTainted(src)) {
                dataFlow.propagateTaint(dest, src);
            }
        }
        return true;
    }

    // INT001: new Type[n * size]
    bool VisitCXXNewExpr(CXXNewExpr *newExpr) {
        if (newExpr->isArray()) {
            if (auto arraySizeOpt = newExpr->getArraySize()) {
                Expr *sizeExpr = *arraySizeOpt;
                if (isRiskyArithmetic(sizeExpr, context)) {
                    SourceManager &sm = context->getSourceManager();
                    SourceLocation loc = sm.getExpansionLoc(newExpr->getExprLoc());
                    int line = sm.getSpellingLineNumber(loc);
                    addFinding({
                        filePath,
                        line,
                        "INT001",
                        "MEDIUM",
                        "Potential integer overflow: arithmetic expression in array new[] size without overflow validation.",
                        getCurrentFunctionName(),
                        "Validate arithmetic operands or check for overflow before allocating memory."
                    });
                }
            }
        }
        return true;
    }
};

class SecurityConsumer : public ASTConsumer {
private:
    string filePath;
    vector<Finding> &findings;
    CallGraph &callGraph;
    DataFlowTracker &dataFlow;

public:
    SecurityConsumer(const string &filePath,
                     vector<Finding> &findings,
                     CallGraph &callGraph,
                     DataFlowTracker &dataFlow)
        : filePath(filePath),
          findings(findings),
          callGraph(callGraph),
          dataFlow(dataFlow) {}

    void HandleTranslationUnit(ASTContext &context) override {
        SecurityVisitor visitor(&context, filePath, findings, callGraph, dataFlow);
        visitor.TraverseDecl(context.getTranslationUnitDecl());

        // Interprocedural vulnerability analysis from CallGraph
        vector<Finding> interproceduralFindings = callGraph.analyzeInterproceduralVulnerabilities();
        findings.insert(findings.end(), interproceduralFindings.begin(), interproceduralFindings.end());

        // Data-flow vulnerability analysis (source-to-sink)
        vector<Finding> dataFlowFindings = dataFlow.generateFindings();
        findings.insert(findings.end(), dataFlowFindings.begin(), dataFlowFindings.end());
    }
};

class SecurityAction : public ASTFrontendAction {
private:
    string filePath;
    vector<Finding> &findings;
    CallGraph &callGraph;
    DataFlowTracker &dataFlow;

public:
    SecurityAction(const string &filePath,
                   vector<Finding> &findings,
                   CallGraph &callGraph,
                   DataFlowTracker &dataFlow)
        : filePath(filePath),
          findings(findings),
          callGraph(callGraph),
          dataFlow(dataFlow) {}

    unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &compiler,
                                             StringRef file) override {
        return make_unique<SecurityConsumer>(filePath, findings, callGraph, dataFlow);
    }
};

class SecurityActionFactory : public FrontendActionFactory {
private:
    string filePath;
    vector<Finding> &findings;
    CallGraph &callGraph;
    DataFlowTracker &dataFlow;

public:
    SecurityActionFactory(const string &filePath,
                          vector<Finding> &findings,
                          CallGraph &callGraph,
                          DataFlowTracker &dataFlow)
        : filePath(filePath),
          findings(findings),
          callGraph(callGraph),
          dataFlow(dataFlow) {}

    unique_ptr<FrontendAction> create() override {
        return make_unique<SecurityAction>(filePath, findings, callGraph, dataFlow);
    }
};

bool analyzeFileWithASTDetailed(
    const string &filePath,
    vector<Finding> &findings,
    CallGraph &callGraph,
    DataFlowTracker &dataFlow)
{
    string errorMessage;
    string absolutePath = filesystem::absolute(filePath).lexically_normal().string();

    unique_ptr<CompilationDatabase> database;
    if (filesystem::exists("build/compile_commands.json")) {
        database = CompilationDatabase::loadFromDirectory("build", errorMessage);
    } else if (filesystem::exists("compile_commands.json")) {
        database = CompilationDatabase::loadFromDirectory(".", errorMessage);
    }

    bool hasCommand = false;
    if (database) {
        auto cmds = database->getCompileCommands(absolutePath);
        if (!cmds.empty()) {
            hasCommand = true;
        }
    }

    unique_ptr<CompilationDatabase> fallbackDb;
    if (!hasCommand) {
        vector<string> syntaxArgs = {
            "-std=c++17",
            "-w",
            "-I.",
            "-Iinclude",
            "-Isrc"
        };
        fallbackDb = make_unique<FixedCompilationDatabase>(".", syntaxArgs);
    }

    CompilationDatabase &activeDb = hasCommand ? *database : *fallbackDb;
    ClangTool tool(activeDb, vector<string>{absolutePath});

    // Suppress internal clang warnings to prevent terminal clutter
    tool.setDiagnosticConsumer(new IgnoringDiagConsumer());

    SecurityActionFactory factory(absolutePath, findings, callGraph, dataFlow);
    int result = tool.run(&factory);

    return result == 0;
}

bool analyzeFileWithAST(
    const string &filePath,
    vector<Finding> &findings)
{
    CallGraph callGraph;
    DataFlowTracker dataFlow;
    return analyzeFileWithASTDetailed(filePath, findings, callGraph, dataFlow);
}

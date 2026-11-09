#include <iostream>
#include <string>

#include <clang/AST/AST.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/Frontend/ASTUnit.h>
#include <clang/Tooling/Tooling.h>

using namespace std;
using namespace clang;
using namespace clang::tooling;

class FunctionCallVisitor : public RecursiveASTVisitor<FunctionCallVisitor> {
private:
    ASTContext *context;

public:
    explicit FunctionCallVisitor(ASTContext *context)
        : context(context) {}

    bool VisitCallExpr(CallExpr *call) {

        if (FunctionDecl *function = call->getDirectCallee()) {

            string name = function->getNameAsString();

            if (name == "strcpy" ||
                name == "gets" ||
                name == "sprintf") {

                SourceManager &sourceManager =
                    context->getSourceManager();

                SourceLocation location = call->getExprLoc();

                unsigned line =
                    sourceManager.getSpellingLineNumber(location);

                cout << "[HIGH] "
                     << name
                     << " detected at line "
                     << line
                     << endl;
            }
        }

        return true;
    }
};

class FunctionCallConsumer : public ASTConsumer {
public:
    void HandleTranslationUnit(ASTContext &context) override {

        FunctionCallVisitor visitor(&context);

        visitor.TraverseDecl(context.getTranslationUnitDecl());
    }
};

class FunctionCallAction : public ASTFrontendAction
{
public:
    unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance &compiler,
        StringRef file) override
    {

        return make_unique<FunctionCallConsumer>();
    }
};

int main()
{

    string code = R"CODE(
        #include <iostream>
        #include <cstring>

        int main() {
            char buffer[20];
            strcpy(buffer, "hello");
            return 0;
        }
    )CODE";

    bool success = runToolOnCode(
        make_unique<FunctionCallAction>(),
        code);

    return success ? 0 : 1;
}
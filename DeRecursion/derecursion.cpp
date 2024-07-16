#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "llvm/Support/raw_ostream.h"
#include "clang/Lex/Lexer.h"
#include "clang/AST/Stmt.h"
#include "clang/AST/StmtVisitor.h"

using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;

static llvm::cl::OptionCategory ToolingSampleCategory("Tooling Sample");
static llvm::cl::opt<std::string> InputFilename(llvm::cl::Positional, llvm::cl::desc("<input file>"),
                                                llvm::cl::init("-"), llvm::cl::cat(ToolingSampleCategory));
static llvm::cl::opt<std::string> OutputFilename("O", llvm::cl::desc("Specify output filename"),
                                                 llvm::cl::value_desc("filename"), llvm::cl::init("fix.cpp"),
                                                 llvm::cl::cat(ToolingSampleCategory));

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
    MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

public:
    bool VisitCallExpr(CallExpr *callExpr) {
        if (!isRecursion) {
            if (currentFunction == nullptr) {
                return true;
            } else if (callExpr->getDirectCallee() == currentFunction) {
                isRecursion = true;
                std::cout << "There is recursion in function " << currentFunction->getNameAsString() << std::endl;
            }
        }
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *funcDecl) {
        isRecursion = false;
        currentFunction = funcDecl;
        TraverseStmt(funcDecl->getBody());
        currentFunction = nullptr;
        return false;
    }

private:
    FunctionDecl *currentFunction = nullptr;
    bool isRecursion = false;
    unsigned int rewriterFunctionCount = 0;
private:
    Rewriter &TheRewriter;
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
    MyASTConsumer(Rewriter &R) : Visitor(R) {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    bool HandleTopLevelDecl(DeclGroupRef DR) override {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b) {
            // Traverse the declaration using our AST visitor.
            Visitor.TraverseDecl(*b);
        }
        return true;
    }

private:
    MyASTVisitor Visitor;
};

// For each source file provided to the tool, a new FrontendAction is created.
class MyFrontendAction : public ASTFrontendAction {
public:
    MyFrontendAction() {}

    void EndSourceFileAction() override {
        SourceManager &SM = TheRewriter.getSourceMgr();
        std::error_code EC;
        llvm::raw_fd_ostream OS(OutputFilename, EC, llvm::sys::fs::OF_Text);
        if (EC) {
            llvm::errs() << "Error opening output file: " << EC.message() << "\n";
            return;
        }
        TheRewriter.getEditBuffer(SM.getMainFileID()).write(OS);
        OS.close();
    }

    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
        TheRewriter.setSourceMgr(CI.getSourceManager(), CI.getLangOpts());
        return std::make_unique<MyASTConsumer>(TheRewriter);
    }

private:
    Rewriter TheRewriter;
};

int main(int argc, const char **argv) {
    llvm::Expected<clang::tooling::CommonOptionsParser> op = CommonOptionsParser::create(argc, argv,
                                                                                         ToolingSampleCategory);
    ClangTool Tool(op->getCompilations(), op->getSourcePathList());
    return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
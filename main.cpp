//------------------------------------------------------------------------------
// Tooling sample. Demonstrates:
//
// * How to write a simple source tool using libTooling.
// * How to use RecursiveASTVisitor to find interesting AST nodes.
// * How to use the Rewriter API to rewrite the source code.
//
// Eli Bendersky (eliben@gmail.com)
// This code is in the public domain
//------------------------------------------------------------------------------
#include <sstream>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include "clang/AST/AST.h"
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
static llvm::cl::opt<std::string> OutputFilename("O", llvm::cl::desc("Specify output filename"), llvm::cl::value_desc("filename"),llvm::cl::init("fix.cpp"));
static llvm::cl::opt<unsigned int> MaxLoopLayerNumber("N", llvm::cl::desc("The biggest number of layer recognized"), llvm::cl::value_desc("MaxLayerNumber"),llvm::cl::init(1));

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
  MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

public:
    unsigned int getInnerLoopLayerNumber(ForStmt *forStmt){
        unsigned int maxInnerLoopLayerNumber = 0;
        for (Stmt *subStmt : forStmt->getBody()->children()) {
            if (isa<ForStmt>(subStmt)) {
                maxInnerLoopLayerNumber=std::max(getInnerLoopLayerNumber(dyn_cast<ForStmt>(subStmt)),maxInnerLoopLayerNumber);
            }
        }
        return maxInnerLoopLayerNumber+1;
    }
    bool VisitDeclStmt(DeclStmt* declStmt){
      if(isValueSearch){
          if (declStmt->isSingleDecl()) {
              if (auto *VD = dyn_cast<VarDecl>(declStmt->getSingleDecl())) {
                  std::string name = VD->getNameAsString();
                  std::string type = VD->getType().getAsString();
                  innerInitialParams.emplace_back(type, name);
              }
          }
      }
      return true;
    }
    bool VisitDeclRefExpr(DeclRefExpr *declRefExpr) {
        if (isValueSearch) {
            std::string type = declRefExpr->getType().getAsString();
            std::string name = declRefExpr->getNameInfo().getAsString();
            if(std::find(innerInitialParams.begin(),innerInitialParams.end(),std::make_pair(type,name))==innerInitialParams.end())
            params.emplace_back(type, name);
        }
        return true;
    }
    bool VisitForStmt(ForStmt *forStmt) {
        // Check if this ForStmt contains another ForStmt
        unsigned int LoopNumber=getInnerLoopLayerNumber(forStmt);

        // If there are no nested ForStmts, this is the innermost loop
        if (LoopNumber<=MaxLoopLayerNumber&&currentFunction&&!isValueSearch) {
                llvm::outs() << "Found an innermost for loop inside function: "
                             << currentFunction->getNameInfo().getName().getAsString() << "\n";
                if (CompoundStmt *CS = dyn_cast<CompoundStmt>(forStmt->getBody())) {
                    std::string innermostLoopBody;
                    for (Stmt *innerStmt : CS->body()) {
                        SourceLocation startLoc = innerStmt->getBeginLoc();
                        SourceLocation endLoc = Lexer::getLocForEndOfToken(innerStmt->getEndLoc(), 0,
                                                                           TheRewriter.getSourceMgr(),
                                                                           TheRewriter.getLangOpts());
                        llvm::StringRef stmtStr = Lexer::getSourceText(CharSourceRange::getTokenRange(startLoc, endLoc),
                                                                       TheRewriter.getSourceMgr(),
                                                                       TheRewriter.getLangOpts());
                        innermostLoopBody += stmtStr.str() + "\n";

                        isValueSearch = true;
                        TraverseStmt(innerStmt);
                        isValueSearch = false;
                    }
                    // Remove duplicate parameters

                    std::sort(params.begin(), params.end());
                    params.erase(std::unique(params.begin(), params.end()), params.end());

                    // Create a new function for the innermost loop body
                    std::string newFunctionName = currentFunction->getNameInfo().getName().getAsString() + "_innerLoop"+ std::to_string(rewriterFunctionCount++);

                    // Add parameters to the new function
                    std::string paramList;
                    std::string paramNames;
                    for (const auto &param : params) {
                        paramList += param.first + " " + param.second + ", ";
                        paramNames += param.second + ", ";
                    }
                    if (!paramList.empty()) {
                        paramList.pop_back();
                        paramList.pop_back(); // Remove the trailing comma and space
                    }
                    if (!paramNames.empty()) {
                        paramNames.pop_back();
                        paramNames.pop_back(); // Remove the trailing comma and space
                    }

                    std::string newFunction = "void " + newFunctionName + "(" + paramList + ") {\n" + innermostLoopBody + "}\n";

                    // Insert the new function definition at the beginning of the file
                    SourceLocation funcStartLoc = TheRewriter.getSourceMgr().getLocForStartOfFile(
                            TheRewriter.getSourceMgr().getMainFileID());
                    TheRewriter.InsertText(funcStartLoc, newFunction + "\n", true, true);

                    // Replace the innermost loop body with a call to the new function
                    std::string newFunctionCall = newFunctionName + "(" + paramNames + ");";
                    TheRewriter.ReplaceText(forStmt->getBody()->getSourceRange(), newFunctionCall);

                return false;
            }
        }
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *funcDecl) {
        currentFunction = funcDecl;
        TraverseStmt(funcDecl->getBody());
        currentFunction = nullptr;
        return true;
    }

private:
    FunctionDecl *currentFunction = nullptr;
    bool isValueSearch = false;
    std::vector<std::pair<std::string, std::string>> params;
    std::vector<std::pair<std::string, std::string>> innerInitialParams;
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
//      (*b)->dump();
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
  llvm::Expected<clang::tooling::CommonOptionsParser> op = CommonOptionsParser::create(argc, argv, ToolingSampleCategory);
  ClangTool Tool(op->getCompilations(), op->getSourcePathList());
  return Tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
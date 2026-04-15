#include <cmath>
#include <iostream>
#include <vector>

#include <stdio.h>

#include "antlr4-runtime.h"

#include "SimpleIRBaseListener.h"
#include "SimpleIRLexer.h"
#include "SimpleIRParser.h"

using namespace antlr4;
using namespace std;

const int bytewidth = 8;
static const char *registers[] = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};

const char *filename;
map<string, int> symtab;
int stackoffset = 0;

static string operand_to_string(antlr4::Token *token) {
  stringstream operand;
  if (SimpleIRParser::NAME == token->getType()) {
    operand << symtab[token->getText()] << "(%rbp)";
  } else if (SimpleIRParser::NUM == token->getType()) {
    operand << "$" << token->getText();
  } else {
    assert(false);
  }
  return operand.str();
}

class CodeGen : public SimpleIRBaseListener {
public:
  virtual void enterUnit(SimpleIRParser::UnitContext *ctx) override {
    // filename
    cout << "\t.file \"" << filename << "\"" << endl;
    cout << "\t.section .note.GNU-stack,\"\",@progbits" << endl;
    cout << "\t.text" << endl;
  }

  virtual void enterFunction(SimpleIRParser::FunctionContext *ctx) override {
    cout << "\t.globl main" << endl;
    cout << "\t.type main" << ", @function" << endl;
    cout << "main:" << endl;
    cout << "\t# prologue, update stack pointer" << endl;
    cout << "\tpushq\t%rbp # save old base ponter" << endl;
    cout << "\tmovq\t%rsp, %rbp # set new base pointer" << endl;
    cout << "\tpush\t%rbx # %rbx is callee-saved" << endl;
  }

  virtual void enterEnd(SimpleIRParser::EndContext *ctx) override {
    cout << "\t# epilogue" << endl;
    cout << "\tadd\t$" << stackoffset << ", %rsp" << endl;
    cout << "\tpop\t%rbx # restore %rbx" << endl;
    cout << "\tpop\t%rbp # restore old base pointer" << endl;
    cout << "\tret" << endl;
  }

  virtual void
  enterLocalVariables(SimpleIRParser::LocalVariablesContext *ctx) override {
    auto variables = ctx->variables;
    // start from of an offset of 2x bytewidth from rbp, 1 to start after rbp
    // and another 1 to skip rbx which will have already been pushed
    int starting_offset = 2;
    for (int variable_i = 0; variable_i < variables.size(); variable_i++) {
      string variable = variables[variable_i]->getText();
      int offset = (starting_offset + variable_i) * bytewidth;
      symtab[variable] = -1 * offset;
    }
    // compute size of the stack space needed
    int stackspace = variables.size() * bytewidth;
    // ceiling to 8 bytes
    stackoffset = std::ceil(stackspace / 8) * 8;
    // align to 16 bytes, accounting for rbx being pushed by adding 8
    stackoffset += (stackoffset + 8) % 16;
    // emit the subtraction to allocate stack space
    cout << "\t# allocate stack space for locals" << endl;
    cout << "\tsub\t$" << stackoffset << ", %rsp" << endl;
  }

  virtual void
  enterParameters(SimpleIRParser::ParametersContext *ctx) override {}

  virtual void
  enterReturnStatement(SimpleIRParser::ReturnStatementContext *ctx) override {
    cout << "\t# set return value" << endl;
    cout << "\tmov\t$0, %rax" << endl;
  }

  virtual void enterStatement(SimpleIRParser::StatementContext *ctx) override {}

  virtual void enterAssign(SimpleIRParser::AssignContext *ctx) override {
    string operand = operand_to_string(ctx->operand);
    cout << "\t# assign " << ctx->operand->getText() << " to "
         << ctx->variable->getText() << endl;
    cout << "\tmov\t" << operand << ", %rax" << endl;
    cout << "\tmov\t%rax, " << symtab[ctx->variable->getText()] << "(%rbp)"
         << endl;
  }

  virtual void enterOperation(SimpleIRParser::OperationContext *ctx) override {}

  virtual void enterCall(SimpleIRParser::CallContext *ctx) override {
    cout << "\tmov\t-72(%rbp), %rdi" << endl;
    cout << "\tcall\tprint_int" << endl;
    cout << "\tadd\t$0, %rsp" << endl;
    cout << "\tmov\t%rax, -80(%rbp)" << endl;
  }

  virtual void enterLabel(SimpleIRParser::LabelContext *ctx) override {}

  virtual void
  enterGotoStatement(SimpleIRParser::GotoStatementContext *ctx) override {}

  virtual void enterIfGoto(SimpleIRParser::IfGotoContext *ctx) override {}

  virtual void
  enterDereference(SimpleIRParser::DereferenceContext *ctx) override {
    cout << "\t# set " << ctx->variable->getText()
         << " to value at memory address in " << ctx->operand->getText()
         << endl;

    cout << "\tmov\t" << symtab[ctx->operand->getText()] << "(%rbp)"
         << ", %rax" << endl;
    cout << "\tmov\t(%rax), %rbx" << endl;
    cout << "\tmov\t%rbx, " << symtab[ctx->variable->getText()] << "(%rbp)"
         << endl;
  }

  virtual void enterReference(SimpleIRParser::ReferenceContext *ctx) override {
    cout << "\t# set " << ctx->variable->getText() << " to "
         << ctx->operand->getText() << " memory address" << endl;

    cout << "\tmov\t%rbp, %rax" << endl;
    cout << "\tadd\t$" << symtab[ctx->operand->getText()] << ", %rax" << endl;
    cout << "\tmov\t%rax, " << symtab[ctx->variable->getText()] << "(%rbp)"
         << endl;
  }

  virtual void enterAssignDereference(
      SimpleIRParser::AssignDereferenceContext *ctx) override {
    cout << "\t# store at memory address in " << ctx->variable->getText()
         << " value of " << ctx->operand->getText() << endl;

    cout << "\tmov\t" << symtab[ctx->variable->getText()] << "(%rbp)"
         << ", %rax" << endl;
    cout << "\tmov\t" << operand_to_string(ctx->operand) << ", %rbx" << endl;
    cout << "\tmov\t%rbx, (%rax)" << endl;
  }
};

int main(int argc, const char *argv[]) {
  ANTLRInputStream *input;

  if (argc > 1) {
    ifstream fstream;
    filename = argv[1];
    fstream.open(filename);
    ANTLRInputStream *finput = new ANTLRInputStream(fstream);
    input = finput;
  } else {
    filename = "stdin";
    input = new ANTLRInputStream(cin);
  }

  SimpleIRLexer lexer(input);
  CommonTokenStream tokens(&lexer);
  SimpleIRParser parser(&tokens);

  tree::ParseTree *tree = parser.unit();
  CodeGen listener;
  tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);

  return 0;
}

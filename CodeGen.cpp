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
    // Start 2 bytes after rbp
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
  enterParameters(SimpleIRParser::ParametersContext *ctx) override {
    // empty for template.ir
  }

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

  virtual void enterOperation(SimpleIRParser::OperationContext *ctx) override {
    auto op_type = ctx->operatorKind->getType();

    string operand_1 = operand_to_string(ctx->operand1);
    string operand_2 = operand_to_string(ctx->operand2);

    cout << "\tmov\t" << operand_1 << ", %rax" << endl;
    cout << "\tmov\t" << operand_2 << ", %rbx" << endl;

    switch (op_type) {
    case SimpleIRParser::PLUS:
      cout << "\tadd %rbx, %rax" << endl;
      break;
    case SimpleIRParser::MINUS:
      cout << "\tsub %rbx, %rax" << endl;
      break;
    case SimpleIRParser::STAR:
      cout << "\timul %rbx, %rax" << endl;
      break;
    case SimpleIRParser::SLASH:
      cout << "\tcdq" << endl;
      cout << "\tidiv %rbx" << endl;
      break;
    case SimpleIRParser::PERCENT:
      cout << "\tcdq" << endl;
      cout << "\tidiv %rbx" << endl;

      // Move remainder to variable
      cout << "\tmov %rdx, " << symtab[ctx->variable->getText()] << "(%rbp)"
           << endl;
      return;
    }

    cout << "\tmov %rax, " << symtab[ctx->variable->getText()] << "(%rbp)"
         << endl;
  }

  virtual void enterCall(SimpleIRParser::CallContext *ctx) override {
    cout << "\tmov -72(%rbp), %rdi" << endl;
    cout << "\tcall print_int" << endl;
    cout << "\tadd $0, %rsp" << endl;
    cout << "\tmov %rax, -80(%rbp)" << endl;
  }

  virtual void enterLabel(SimpleIRParser::LabelContext *ctx) override {
    cout << ctx->labelName->getText() << ":" << endl;
  }

  virtual void
  enterGotoStatement(SimpleIRParser::GotoStatementContext *ctx) override {
    cout << "\tjmp\t" << ctx->labelName->getText() << endl;
  }

  virtual void enterIfGoto(SimpleIRParser::IfGotoContext *ctx) override {
    auto comp_type = ctx->operatorKind->getType();
    string operand_1 = operand_to_string(ctx->operand1);
    string operand_2 = operand_to_string(ctx->operand2);
    string jump_target = ctx->labelName->getText();

    cout << "\tmov\t" << operand_1 << ", %rax" << endl;
    cout << "\tmov\t" << operand_2 << ", %rbx" << endl;
    cout << "\tcmp %rbx, %rax" << endl;

    switch (comp_type) {
    case SimpleIRParser::EQ:
      cout << "\tje\t" << jump_target << endl;
      break;
    case SimpleIRParser::NEQ:
      cout << "\tjne\t" << jump_target << endl;
      break;
    case SimpleIRParser::LT:
      cout << "\tjl\t" << jump_target << endl;
      break;
    case SimpleIRParser::LTE:
      cout << "\tjle\t" << jump_target << endl;
      break;
    case SimpleIRParser::GT:
      cout << "\tjg\t" << jump_target << endl;
      break;
    case SimpleIRParser::GTE:
      cout << "\tjge\t" << jump_target << endl;
      break;
    }
  }

  virtual void
  enterDereference(SimpleIRParser::DereferenceContext *ctx) override {}

  virtual void enterReference(SimpleIRParser::ReferenceContext *ctx) override {}

  virtual void enterAssignDereference(
      SimpleIRParser::AssignDereferenceContext *ctx) override {}
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

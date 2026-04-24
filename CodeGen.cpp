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
    cout << "\t.globl " << ctx->functionName->getText() << endl;
    cout << "\t.type " << ctx->functionName->getText() << ", @function" << endl;
    cout << ctx->functionName->getText() << ":" << endl;
    cout << "\t# prologue" << endl;
    cout << "\tpushq %rbp" << endl;
    cout << "\tmovq %rsp, %rbp" << endl;
    cout << "\tpushq %rbx" << endl;
  }

  virtual void enterEnd(SimpleIRParser::EndContext *ctx) override {
    cout << "# epilogue" << endl;
    cout << "\tadd" << "\t $" << stackoffset << ", %rsp" << endl;
    cout << "\tpop %rbx" << endl;
    cout << "\tpop %rbp" << endl;
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
  enterParameters(SimpleIRParser::ParametersContext *ctx) override {
    auto formals = ctx->formals;
    for (int formal_i = 0; formal_i < formals.size(); formal_i++) {
      int base_ptr_offset = (formal_i + 2) * -8;

      if (formal_i > 5) {
        int stack_offset = (formal_i - 4) * 8;

        cout << "\t# move stack parameter " << formals[formal_i]->getText()
             << " to local variable" << endl;
        cout << "\tmov\t" << stack_offset << "(%rbp), %rax" << endl;
        cout << "\tmov\t %rax, " << base_ptr_offset << "(%rbp)" << endl;
      } else {
        cout << "\t# move register parameter " << formals[formal_i]->getText()
             << " to local variable" << endl;

        cout << "\tmov\t" << registers[formal_i] << ", " << base_ptr_offset
             << "(%rbp)" << endl;
      }
    }
  }

  virtual void
  enterReturnStatement(SimpleIRParser::ReturnStatementContext *ctx) override {
    cout << "\t# set return value" << endl;
    cout << "\tmov\t " << operand_to_string(ctx->operand) << ", %rax" << endl;
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
    cout << "\t# calling " << ctx->functionName->getText() << endl;

    auto actuals = ctx->actuals;
    int total_arguments = actuals.size();
    int stack_arg_count = total_arguments > 6 ? total_arguments - 6 : 0;

    for (int i = total_arguments - 1; i >= total_arguments - 6; i--) {
      cout << "\tmov\t " << operand_to_string(actuals[i]) << ", "
           << registers[total_arguments - i - 1] << endl;
    }

    for (int i = stack_arg_count; i > 0; i--) {
      cout << "\tpush\t " << operand_to_string(actuals[i + 1]) << endl;
    }

    // # make the call
    cout << "\tcall\t " << ctx->functionName->getText() << endl;

    if (stack_arg_count > 0) { // # restore the stack pointer
      cout << "\tadd $" << stack_arg_count * 8 << ", %rsp" << endl;
    }

    // # save the return value
    cout << "\tmov %rax, " << operand_to_string(ctx->variable) << endl;
  }

  virtual void enterLabel(SimpleIRParser::LabelContext *ctx) override {}

  virtual void
  enterGotoStatement(SimpleIRParser::GotoStatementContext *ctx) override {}

  virtual void enterIfGoto(SimpleIRParser::IfGotoContext *ctx) override {}

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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include "ast.h"
#include "SymbolTable.h"
#include "c.tab.hpp"

extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;

static void usage()
{
  printf("Usage: cc <prog.c> [-v]\n");
}

using namespace std;
using namespace ast;

yyTU *topLevelTU = new yyTU();

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);

  bool verbose = false;
  if (argc > 2)
  {
    if (strcmp(argv[2], "-v") == 0)
    {
      verbose = true;
    }
    else
    {
      usage();
      exit(1);
    }
  }

  int ret = yyparse();

  if (ret != 0)
  {
    std::cerr << "Compilation failed!" << std::endl;
  }
  else
  {
    SymbolTable<yyAST *> *symTable = new SymbolTable<yyAST *>();
    if (!topLevelTU->envCheck(symTable))
    {
      std::cerr << "Compilation failed!" << std::endl;
    }
    else
    {
      assert(symTable->table.size() == 1);
      // if (verbose)
      // {
      //   topLevelTU->print();
      // }
      SymbolTable<yyAST *> *symTable = new SymbolTable<yyAST *>();

      if (!topLevelTU->typeCheck(symTable))
      {
        std::cerr << "Compilation failed!" << std::endl;
      }
      else
      {
        assert(symTable->table.size() == 1);

        if (verbose)
        {
          for (auto symbol : symTable->table[0])
          {
            std::cout << symbol.first << ": " << symbol.second->my_type->typeStr() << "\n";
          }
          std::cout << "Compilation successful!" << std::endl;
          topLevelTU->print();
        }

        CodeGenContext *context = new CodeGenContext();

        topLevelTU->codeGen(context);

        if (!verifyModule(*context->module, &errs()))
        {
          if (verbose)
          {
            std::cout << "Code generation successful!" << std::endl;
          }
          // only print the code in non-verbose mode
          context->module->print(errs(), nullptr);
        }
        else
        {
          std::cerr << "Code generation failed!" << std::endl;
        }
        std::cout << std::flush;
      }
    }
  }
}

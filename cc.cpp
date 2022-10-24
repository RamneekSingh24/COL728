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

extern yyTU *topLevelTU;

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

  //  cout << (topLevelTU->nodes.size()) << "\n";

  //  for (auto node : topLevelTU->nodes) {
  //      std:: cout << node->name() << "\n";
  //  }

  //  std::cout << topLevelTU->nodes[4]->nodes[1]->nodes[1]->nodes[0]->nodes[1]->nodes[0]->name() << "\n";

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
      if (verbose)
      {
        topLevelTU->print();
      }
      SymbolTable<yyAST *> *symTable = new SymbolTable<yyAST *>();

      if (!topLevelTU->typeCheck(symTable))
      {
        std::cerr << "Compilation failed!" << std::endl;
      }
      else
      {
        assert(symTable->table.size() == 1);
        for (auto symbol : symTable->table[0])
        {
          std::cout << symbol.first << ": " << symbol.second->my_type->typeStr() << "\n";
        }
        if (verbose)
        {
          std::cout << "Compilation successful!" << std::endl;
          topLevelTU->print();
        }
        std::cout << std::flush;
      }
    }
  }
}

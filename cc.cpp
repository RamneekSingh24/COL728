#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include "ast.h"
#include "c.tab.hpp"



extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;


static void usage()
{
  printf("Usage: cc <prog.c>\n");
}

using namespace std;
using namespace ast;

extern yyTU* topLevelTU;




int main(int argc, char **argv)
{

  if (argc != 2) {
    usage();
    exit(1);
  }
  char const *filename = argv[1];
  yyin = fopen(filename, "r");
  assert(yyin);

  int ret = yyparse();

//  cout << (topLevelTU->nodes.size()) << "\n";

//  for (auto node : topLevelTU->nodes) {
//      std:: cout << node->name() << "\n";
//  }

//  std::cout << topLevelTU->nodes[4]->nodes[1]->nodes[1]->nodes[0]->nodes[1]->nodes[0]->name() << "\n";

  if (ret != 0) {
      std::cerr << "Compilation failed!" << std::endl;
  }
  else {
      topLevelTU->print();
      std::cout << std::flush;
  }


}

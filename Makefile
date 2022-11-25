# LLVM_CONFIG=/opt/homebrew/Cellar/llvm/15.0.3/bin/llvm-config
# BISON=/opt/homebrew/Cellar/bison/3.8.2/bin/bison
# TODO ^^ FIX before submission

LLVM_CONFIG=llvm-config
BISON=bison

LLVM_OPTS=`${LLVM_CONFIG} --cxxflags --ldflags --system-libs --libs core`

CC=clang++


cc: cc.cpp c.tab.cpp c.lex.cpp ast.h SymbolTable.cpp SymbolTable.h code_optimization.cpp code_optimization.h
	${CC} -std=c++17 ${LLVM_OPTS} c.tab.cpp c.lex.cpp cc.cpp SymbolTable.cpp code_optimization.cpp -lm -ll  -o $@


c.tab.cpp c.tab.hpp: c.y
	${BISON} -o c.tab.cpp -d c.y

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

clean:
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output

parser: c.y ast.h
	 -o c.tab.cpp -d c.y -Wcounterexamples


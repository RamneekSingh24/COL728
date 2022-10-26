LLVM_CONFIG=/opt/homebrew/Cellar/llvm/15.0.3/bin/llvm-config
BISON=/opt/homebrew/Cellar/bison/3.8.2/bin/bison
# TODO ^^ FIX before submission
LLVM_OPTS=`${LLVM_CONFIG} --cxxflags --ldflags --system-libs --libs core`

CC=clang++


cc: cc.cpp c.tab.cpp c.lex.cpp ast.h SymbolTable.cpp SymbolTable.h
	${CC} -g  -std=c++17 ${LLVM_OPTS} c.tab.cpp c.lex.cpp cc.cpp SymbolTable.cpp -lm -ll  -o $@

dbg:
	./cc examples/test1.c


c.tab.cpp c.tab.hpp: c.y
	${BISON} -o c.tab.cpp -d c.y
    # TODO fix before submission

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

clean::
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output && rm -rf .venv sum.o sum sum.bc sum.ll

parser: c.y ast.h
	 -o c.tab.cpp -d c.y -Wcounterexamples

make venv:
	python3 -m venv .venv

make install:
	brew install graphviz
	source .venv/bin/activate && pip3 install anytree


make viz: cc
	source .venv/bin/activate && ./cc ${PROG} | python3 visualize_ast.py && open viz.png



toy: toy.cpp
	clang++ -g -O3 toy.cpp `${LLVM_CONFIG} --cxxflags --ldflags --system-libs --libs core` -o toy

sum: sum.cpp
	clang++ -g -O3 sum.cpp `${LLVM_CONFIG} --cxxflags --ldflags --system-libs --libs core` -o sum



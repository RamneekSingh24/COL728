cc: cc.cpp c.tab.cpp c.lex.cpp ast.h SymbolTable.cpp SymbolTable.h
	g++ -g -std=c++17 c.tab.cpp c.lex.cpp cc.cpp SymbolTable.cpp -lm -ll  -o $@

dbg:
	./cc examples/test1.c


c.tab.cpp c.tab.hpp: c.y
	/opt/homebrew/Cellar/bison/3.8.2/bin/bison -o c.tab.cpp -d c.y
    # TODO fix before submission

c.lex.cpp: c.l c.tab.hpp
	flex -o c.lex.cpp -l c.l

clean::
	rm -f c.tab.cpp c.tab.hpp c.lex.cpp cc c.output && rm -rf .venv

parser: c.y ast.h
	 -o c.tab.cpp -d c.y -Wcounterexamples

make venv:
	python3 -m venv .venv

make install:
	brew install graphviz
	source .venv/bin/activate && pip3 install anytree


make viz: cc
	source .venv/bin/activate && ./cc ${PROG} | python3 visualize_ast.py && open viz.png



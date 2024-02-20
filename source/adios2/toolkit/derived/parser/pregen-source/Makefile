USE_BISON_3.8 = true
CC = g++
PARSER_SOURCE = ../
OPT = -std=c++11 -I$(PARSER_SOURCE) -I./
ifdef USE_BISON_3.8
BISON = /usr/local/Cellar/bison/3.8.2/bin/bison
PARSER = parser.y
else
BISON = /usr/bin/bison
PARSER = parser.y
endif

simple-c: $(PARSER_SOURCE)/driver.cpp $(PARSER_SOURCE)/ASTDriver.cpp $(PARSER_SOURCE)/ASTNode.cpp lexer.cpp parser.cpp
	$(CC) $(OPT) $^ -o $@

lexer.h lexer.cpp: $(PARSER_SOURCE)/lexer.l
	flex --header-file=lexer.h -o lexer.cpp $(PARSER_SOURCE)/lexer.l 

parser.h parser.cpp: $(PARSER_SOURCE)/$(PARSER)
	$(BISON) --version
	$(BISON) -d -v --defines=parser.h -o parser.cpp $(PARSER_SOURCE)/$(PARSER)
ifndef USE_BISON_3.8
	echo "\n#include \"ASTNode.h\"\n\nASTNode* parse_expression(const char* input);" >> parser.h
endif

clean:
	rm *.o *~ lexer.cpp lexer.h parser.cpp parser.h parser.hpp parser.output location.hh simple-c

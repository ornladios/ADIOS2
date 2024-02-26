CC = g++
PARSER_SOURCE = ../
OPT = -std=c++11 -I$(PARSER_SOURCE) -I./
BISON = bison

simple-c: $(PARSER_SOURCE)/parser-test-driver.cpp $(PARSER_SOURCE)/ASTDriver.cpp $(PARSER_SOURCE)/ASTNode.cpp lexer.cpp parser.cpp
	$(CC) $(OPT) $^ -o $@

lexer.h lexer.cpp: $(PARSER_SOURCE)/lexer.l
	flex --header-file=lexer.h -o lexer.cpp $(PARSER_SOURCE)/lexer.l 

parser.h parser.cpp: $(PARSER_SOURCE)/parser.y
	$(BISON) --version
	$(BISON) -d -v --defines=parser.h -o parser.cpp $(PARSER_SOURCE)/parser.y

clean:
	rm *.o *~ lexer.cpp lexer.h parser.cpp parser.h parser.hpp parser.output location.hh simple-c

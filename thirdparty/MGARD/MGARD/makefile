
CXX= g++
CC = gcc

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)
MKDIR=mkdir
RMDIR=rmdir --ignore-fail-on-non-empty


CXXFLAGS= -c  -Wall -Wfatal-errors -I$(INC) -O3 -fPIC
CFLAGS=  -c -Wall -Wfatal-errors -I$(INC) -O3

LDFLAGS = -lz -lm -lstdc++
ARFLAGS = rcs

SRC=src
INC=include
OBJ=obj

vpath %.o $(OBJ)
vpath %.c $(SRC)
vpath %.cpp $(SRC)


SOURCES=mgard_test.c mgard.cpp mgard_nuni.cpp mgard_capi.cpp 
OBJECTS=$(foreach SOURCE,$(basename $(SOURCES)),$(OBJ)/$(SOURCE).o)

SOURCES_SIRIUS=mgard_sirius_test.c mgard.cpp mgard_nuni.cpp mgard_capi.cpp 
OBJECTS_SIRIUS=$(foreach SOURCE,$(basename $(SOURCES_SIRIUS)),$(OBJ)/$(SOURCE).o)

EXECUTABLE=mgard_test
SIRIUS_EXEC=mgard_sirius_test

LIB=libmgard.a

.PHONY: all clean test

all: $(EXECUTABLE) $(LIB) $(SIRIUS_EXEC) test test2 test3

$(EXECUTABLE): $(OBJECTS) 
	$(LINK.o) -o $@ $^

$(SIRIUS_EXEC): $(OBJECTS_SIRIUS) 
	$(LINK.o) -o $@ $^

$(OBJ)/%.o: %.cpp | $(OBJ)
	$(COMPILE.cpp) $< -o $@

$(OBJ)/%.o: %.c | $(OBJ)
	$(COMPILE.c) $< -o $@

$(OBJ):
	$(MKDIR) $@

$(LIB): $(OBJECTS)
	$(AR) $(ARFLAGS) $(LIB) $^

test: $(EXECUTABLE)
	./$(EXECUTABLE) data/u3_513x513_orig data/u3_513x513.mgard  513 513 1e-2

test2: $(EXECUTABLE)
	./$(EXECUTABLE) data/data_600x400_orig data/data_600x400.mgard  600 400 1e-3

test3: $(SIRIUS_EXEC)
	./$(SIRIUS_EXEC) data/data_600x400_orig data/data_600x400_coarse.mgard data/data_600x400_fine.mgard  600 400 1e-2 1e-3

clean:
	$(RM) $(EXECUTABLE) $(OBJECTS) $(LIB) $(SIRIUS_EXEC)
	if [ -d $(OBJ) ]; then $(RMDIR) $(OBJ); fi

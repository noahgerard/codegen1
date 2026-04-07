GRAMMAR = SimpleIR
PROGRAM = CodeGen

# macOS Detection and Dependency Logic
ifeq ($(shell uname), Darwin)
    # macOS Logic: Use Homebrew paths
    ANTLR_PREFIX := $(shell brew --prefix antlr)
    # Extract version from the brew path or jar name
    ANTLR_VERSION = $(shell ls $(ANTLR_PREFIX)/lib/antlr-*-complete.jar | grep -oE '[0-9]+\.[0-9]+' | head -1)
    ANTLR = $(ANTLR_PREFIX)/lib/antlr-$(ANTLR_VERSION)-complete.jar
    
    # Homebrew puts headers and libs in specific spots
    RUNTIME_INCLUDE = $(shell brew --prefix antlr4-cpp-runtime)/include/antlr4-runtime
    RUNTIME_LIB = $(shell brew --prefix antlr4-cpp-runtime)/lib/libantlr4-runtime.dylib
else
    # Original Linux Logic
    ANTLR_VERSION = $(shell dpkg --get-selections | grep antlr4 | grep runtime | egrep -o "([0-9]+\.[0-9]+)" | head -1)
    ANTLR = antlr-$(ANTLR_VERSION)-complete.jar
    RUNTIME_INCLUDE = /usr/include/antlr4-runtime
    RUNTIME_LIB = /usr/lib/x86_64-linux-gnu/libantlr4-runtime.so.$(ANTLR_VERSION)
endif

GRAMMARSRC = $(GRAMMAR).g4
GRAMMARLEXER = $(GRAMMAR).interp $(GRAMMAR)Lexer.interp $(GRAMMAR)Lexer.tokens $(GRAMMAR).tokens
GRAMMARCPP = $(GRAMMAR)Lexer.cpp $(GRAMMAR)Parser.cpp $(GRAMMAR)BaseListener.cpp $(GRAMMAR)Listener.cpp
GRAMMARH = $(patsubst %.cpp, %.h, $(GRAMMARCPP))
SRC = $(GRAMMARCPP) $(PROGRAM).cpp
OBJ = $(patsubst %.cpp, %.o, $(SRC))

.PHONY: all parser clean clobber distclean package

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(CXX) -o $@ $^ $(RUNTIME_LIB)

%.o: %.cpp
	$(CXX) -I $(RUNTIME_INCLUDE) -std=c++17 -c $<

$(ANTLR):
	@echo "ANTLR jar not found at $(ANTLR). Please install via 'brew install antlr'."
	# Note: On macOS, we usually don't wget into the local folder if using Brew.

parser: $(GRAMMARSRC)
	java -jar $(ANTLR) -Dlanguage=Cpp $<

clean:
	rm -f $(OBJ) $(PROGRAM)

clobber: clean
	rm -f $(GRAMMAR).interp $(GRAMMAR)Lexer.interp $(GRAMMAR)Lexer.tokens $(GRAMMAR).tokens
	rm -f $(GRAMMAR)Lexer.h $(GRAMMAR)Parser.h $(GRAMMAR)BaseListener.h $(GRAMMAR)Listener.h
	rm -f $(GRAMMAR)Lexer.cpp $(GRAMMAR)Parser.cpp $(GRAMMAR)BaseListener.cpp $(GRAMMAR)Listener.cpp

distclean: clobber
	# Only remove if it's the local wget version
	rm -f antlr-$(ANTLR_VERSION)-complete.jar

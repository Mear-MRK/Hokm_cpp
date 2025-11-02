# Makefile for Hokm Card Game

# Phony targets
.PHONY: all test clean

# Paths
OBJPATH = ./obj
SRCPATH = ./src
INCPATH = ./include
BINPATH = ./bin

# Compiler and Linker
CXX = clang++
LD = clang++

# Executable names
TARGET = $(BINPATH)/hokm
DBG_TARGET = $(BINPATH)/hokm_dbg
TEST_TARGET = $(BINPATH)/hokm_test
LRN_TARGET = $(BINPATH)/hokm_learn

# Default target
all: $(TARGET) $(DBG_TARGET) $(LRN_TARGET)

# Flags
DEPFLAGS = -MMD -MP
INCFLAGS = -I$(INCPATH)
CXXFLAGS_BASE = -std=c++17 $(INCFLAGS) $(DEPFLAGS)

# Release flags
CXXFLAGS = $(CXXFLAGS_BASE) -DNDEBUG -O2

# Debug flags
DBG_CXXFLAGS = $(CXXFLAGS_BASE) -g -DDEBUG -Wall -Wextra -O2 -DTEST

# Source files
SRC_FILES = \
	Agent.cpp \
	Card.cpp \
	CardStack.cpp \
	Deck.cpp \
	GameRound.cpp \
	Hand.cpp \
	History.cpp \
	InteractiveAgent.cpp \
	InteractiveGame.cpp \
	MultiClientServer.cpp \
	ProbHand.cpp \
	RemoteInterAgent.cpp \
	RndAgent.cpp \
	SoundAgent.cpp \
	State.cpp

MAIN_SRC_FILES = main.cpp

LEARN_SRC_FILES = \
	LearningGame.cpp \
	main_learning.cpp

TEST_SRC_FILES = \
	Card_test.cpp \
	CardStack_test.cpp \
	Deck_test.cpp \
	Hand_test.cpp \
	History_test.cpp \
	State_test.cpp \
	utils_test.cpp \
	main_test.cpp

# Object files
OBJS = $(addprefix $(OBJPATH)/,$(SRC_FILES:.cpp=.o))
MAIN_OBJS = $(addprefix $(OBJPATH)/,$(MAIN_SRC_FILES:.cpp=.o))
DBG_OBJS = $(addprefix $(OBJPATH)/,$(SRC_FILES:.cpp=_dbg.o))
LEARN_OBJS= $(addprefix $(OBJPATH)/,$(LEARN_SRC_FILES:.cpp=.o))
TEST_OBJS = $(addprefix $(OBJPATH)/,$(TEST_SRC_FILES:.cpp=.o))

# Dependencies
DEPS = $(OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Include dependencies
-include $(DEPS)

# --- Targets ---

# Build release executable
$(TARGET): $(OBJS) $(MAIN_OBJS)
	@echo "====== Linking Release Build: $(TARGET) ======"
	@mkdir -p $(dir $@)
	$(LD) -o $@ $^ $(LDFLAGS)

# Build debug executable
$(DBG_TARGET): $(DBG_OBJS) $(OBJPATH)/main_dbg.o
	@echo "====== Linking Debug Build: $(DBG_TARGET) ======"
	@mkdir -p $(dir $@)
	$(LD) -o $@ $^ $(LDFLAGS)

# Build and run tests
test: $(TEST_TARGET)
	@echo "====== Running tests ======"
	./$(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(DBG_OBJS) $(TEST_OBJS)
	@echo "====== Linking Test Build: $(TEST_TARGET) ======"
	@mkdir -p $(dir $@)
	$(LD) -o $@ $^ $(LDFLAGS)

# Build learning executable
$(LRN_TARGET): $(OBJS) $(LEARN_OBJS)
	@echo "====== Linking Learning Build: $(LRN_TARGET) ======"
	@mkdir -p $(dir $@)
	$(LD) -o $@ $^ $(LDFLAGS)


# --- Rules ---

$(OBJPATH)/%.o: $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJPATH)/%_dbg.o: $(SRCPATH)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(DBG_CXXFLAGS) -c $< -o $@

$(OBJPATH)/%_test.o: $(SRCPATH)/%_test.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(DBG_CXXFLAGS) -c $< -o $@


# Clean up
clean:
	@echo "====== Cleaning build artifacts ======"
	rm -rf $(OBJPATH)
	rm -rf $(BINPATH)
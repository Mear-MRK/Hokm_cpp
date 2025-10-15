# Makefile for Hokm Card Game (Linux)

# Phony targets
.PHONY: all test clean

# Default target
all: hokm.out hokm_dbg.out

# Paths
OBJPATH = ./obj
SRCPATH = ./src
INCPATH = ./include

# Compiler and Linker
CXX = g++
LD = g++

# Executable names
TARGET = hokm.out
DBG_TARGET = hokm_dbg.out
TEST_TARGET = hokm_test.out

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
	LearningGame.cpp \
	ProbHand.cpp \
	RemoteInterAgent.cpp \
	RndAgent.cpp \
	SoundAgent.cpp \
	State.cpp \
	main.cpp

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
DBG_OBJS = $(addprefix $(OBJPATH)/,$(SRC_FILES:.cpp=_dbg.o))
TEST_OBJS = $(addprefix $(OBJPATH)/,$(TEST_SRC_FILES:.cpp=.o))

# Dependencies
DEPS = $(OBJS:.o=.d) $(DBG_OBJS:.o=.d) $(TEST_OBJS:.o=.d)

# Include dependencies
-include $(DEPS)

# --- Targets ---

# Build release executable
$(TARGET): $(OBJS)
	@echo "====== Linking Release Build: $(TARGET) ======"
	$(LD) -o $@ $^ $(LDFLAGS)

# Build debug executable
$(DBG_TARGET): $(DBG_OBJS)
	@echo "====== Linking Debug Build: $(DBG_TARGET) ======"
	$(LD) -o $@ $^ $(LDFLAGS)

# Build and run tests
test: $(TEST_TARGET)
	@echo "====== Running tests ======"
	./$(TEST_TARGET)

# Build test executable
$(TEST_TARGET): $(filter-out $(OBJPATH)/main_dbg.o,$(DBG_OBJS)) $(TEST_OBJS)
	@echo "====== Linking Test Build: $(TEST_TARGET) ======"
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

$(OBJPATH)/main_test.o: $(SRCPATH)/main_test.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(DBG_CXXFLAGS) -c $< -o $@


# Clean up
clean:
	@echo "====== Cleaning build artifacts ======"
	rm -rf $(OBJPATH)
	rm -f $(TARGET) $(DBG_TARGET) $(TEST_TARGET)
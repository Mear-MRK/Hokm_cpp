CXX=g++
LD=g++
INCFLAGS=-I/usr/include/eigen3
FLAGS=-std=c++17 -DNDEBUG -O2 $(INCFLAGS)
DBG_FLAGS=-std=c++17 -g -DDEBUG -Wall -Wextra -O2 $(INCFLAGS)

OBJPATH = ./obj
SRCPATH = ./src

SRCFILES = $(filter-out %_test.cpp, $(wildcard $(SRCPATH)/*.cpp))
TSTFILES = $(wildcard $(SRCPATH)/*_test.cpp)
OBJFILES = $(patsubst $(SRCPATH)/%.cpp, $(OBJPATH)/%.o, $(SRCFILES))
DBG_OBJFILES = $(patsubst %.o, %_dbg.o, $(OBJFILES))
TST_OBJFILES = $(patsubst $(SRCPATH)/%.cpp, $(OBJPATH)/%.o, $(TSTFILES))

all: hokm.out hokm_dbg.out
	@echo "====== make all ======"

$(OBJPATH)/%.o: $(SRCPATH)/%.cpp $(SRCPATH)/%.h $(SRCPATH)/GameConfig.h
	@mkdir -p $(OBJPATH)
	$(CXX) $(FLAGS) -o $@ -c $<

$(OBJPATH)/%_dbg.o: $(SRCPATH)/%.cpp $(SRCPATH)/%.h $(SRCPATH)/GameConfig.h
	@mkdir -p $(OBJPATH)
	$(CXX) $(DBG_FLAGS) -o $@ -c $<

$(OBJPATH)/%_test.o: $(SRCPATH)/%_test.cpp $(SRCPATH)/%.h $(SRCPATH)/GameConfig.h
	@mkdir -p $(OBJPATH)
	$(CXX) $(DBG_FLAGS) -o $@ -c $<

$(OBJPATH)/main.o: $(SRCPATH)/main.cpp $(SRCPATH)/GameConfig.h
	@mkdir -p $(OBJPATH)
	$(CXX) $(FLAGS) -o $@ -c $<

$(OBJPATH)/main_dbg.o: $(SRCPATH)/main.cpp $(SRCPATH)/GameConfig.h
	@mkdir -p $(OBJPATH)
	$(CXX) $(DBG_FLAGS) -o $@ -c $<

hokm.out: $(OBJFILES)
	$(LD) -o $@ $^ -lm

hokm_dbg.out: $(DBG_OBJFILES) $(TST_OBJFILES)
	$(LD) -o $@ $^ -lm

hokm_dbg.out: $(DBG_OBJFILES) 

clean:
	rm -rf $(OBJPATH)
	rm -f hokm.out hokm_dbg.out
	@echo "====== make clean ======"

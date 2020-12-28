TARGET ?= fibmgr
SRCDIR = ./src
OBJDIR = ./obj

SRC = $(wildcard $(SRCDIR)/*.cpp)
OBJ = $(addprefix $(OBJDIR)/,$(notdir $(SRC:.cpp=.o)))

SRC_GEN = $(wildcard $(SRCDIR)/fpm/*.cc $(SRCDIR)/qpb/*.cc)
OBJ_GEN =  $(addprefix $(OBJDIR)/,$(notdir $(SRC_GEN:.cpp=.o)))

CPPFLAGS ?= -g -Wall -std=c++17 -Isrc/
LDLIBS += -lpthread -lvapiclient -lboost_system -lmnl -lprotobuf

$(TARGET): $(OBJ_GEN) $(OBJ)
	$(CXX) $(LDFLAGS) $(OBJ) $(OBJ_GEN) -o $@ $(LOADLIBES) $(LDLIBS)

$(OBJDIR):
	mkdir -p $@

$(OBJDIR)/qpb.pb.o: $(SRCDIR)/qpb/qpb.pb.cc
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/fpm.pb.o: $(SRCDIR)/fpm/fpm.pb.cc
	$(CXX) -c $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(OBJDIR)
	$(CXX) -c $(CPPFLAGS) $< -o $@

.PHONY: clean start
clean:
	$(RM) $(TARGET) $(OBJ)

start:
	sudo ./$(TARGET)

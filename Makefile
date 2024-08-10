ifneq "$(wildcard config.mk)" "config.mk"
$(error Error: could not find file `config.mk`. Please make a copy of `config.mk.template`, rename the copy to `config.mk`, and adjust the copy as necessary before attempting to build again.)
endif

# Get all of the locations for dependencies
# Modify dependencies.mk to set the locations/commands for each tool
include config.mk

# Local prefixes
INCLUDE_PREFIX := ./include/
SOURCE_PREFIX := ./source/
TEST_SOURCE_PREFIX := $(SOURCE_PREFIX)/tests/
OBJ_PREFIX := ./obj/
TEST_OBJ_PREFIX := $(OBJ_PREFIX)/tests/
BIN_PREFIX := ./bin/
DEBUG_PREFIX := $(BIN_PREFIX)/Debug/
RELEASE_PREFIX := $(BIN_PREFIX)/Release/
TEST_PREFIX := $(BIN_PREFIX)/Test/

OUTPUT_PREFIX := $(DEBUG_PREFIX)

DEFINES :=

debug: DEFINES += -DDEBUG

# Add more target prefixes here
release: OUTPUT_PREFIX := $(RELEASE_PREFIX)

ifneq ($(COMPLETE_PREREQUISITES), true) #1
COMPLETE_PREREQUISITES := false
endif #1

ifeq ($(COMPLETE_PREREQUISITES), true) #1
AUTO_GENERATE_FLAG := -MM

ifeq ($(SYSTEM_INCLUDE_PREREQUISITE), true) #2
AUTO_GENERATE_FLAG := -M
endif #2

endif #1

INCLUDE := -I $(INCLUDE_PREFIX)

WARNFLAGS := -Wall

CXXFLAGS := -c $(INCLUDE) $(WARNFLAGS) -std=c++20

DEBUG_FLAGS := -g

RELEASE_FLAGS := -O3

debug: CXXFLAGS += $(DEBUG_FLAGS)
release: CXXFLAGS += $(RELEASE_FLAGS)

O_FILES := packets.o packetFactory.o transmission.o protocol.o
O_FILES := $(foreach obj, $(O_FILES), $(OBJ_PREFIX)/$(obj))

TEST_BINS := basicClient mpClient
TEST_OBJS := $(foreach bin, $(TEST_BINS), $(TEST_OBJ_PREFIX)/$(bin).o);
TEST_BINS := $(foreach bin, $(TEST_BINS), $(TEST_PREFIX)/$(bin))

.SECONDARY: $(TEST_OBJS)

.PHONY: clean all debug release cleandeps test

debug: all
release: all
test: debug $(TEST_BINS)

all: | $(OUTPUT_PREFIX)
all: $(OUTPUT_PREFIX)/SMGServer 

clean: cleandeps
	rm -f $(OBJ_PREFIX)/*.o $(DEBUG_PREFIX)/* $(RELEASE_PREFIX)/* $(TEST_PREFIX)/* $(TEST_OBJ_PREFIX)/*.o

cleandeps:
	rm -f $(OBJ_PREFIX)/*.d

$(OBJ_PREFIX):
	mkdir -p $(OBJ_PREFIX)

$(TEST_OBJ_PREFIX):
	mkdir -p $(TEST_OBJ_PREFIX)

$(DEBUG_PREFIX):
	mkdir -p $(DEBUG_PREFIX)

$(RELEASE_PREFIX):
	mkdir -p $(RELEASE_PREFIX)

$(TEST_PREFIX):
	mkdir -p $(TEST_PREFIX)

$(OBJ_PREFIX)/%.o: $(SOURCE_PREFIX)/%.cpp | $(OBJ_PREFIX)
	$(CXX) $(CXXFLAGS) $(DEFINES) -c -o $@ $<

$(OBJ_PREFIX)/%.o: $(SOURCE_PREFIX)/%.c | $(OBJ_PREFIX)
	$(CC) $(CFLAGS) $(DEFINES) -c -o $@ $<

$(TEST_OBJ_PREFIX)/%.o: $(TEST_SOURCE_PREFIX)/%.cpp | $(TEST_OBJ_PREFIX)
	$(CXX) $(CXXFLAGS) $(DEFINES) $(DEBUG_FLAGS) -c -o $@ $<

$(OUTPUT_PREFIX)/SMGServer: $(O_FILES) $(OBJ_PREFIX)/main.o
	$(LD) $(O_FILES) $(OBJ_PREFIX)/main.o -o $@


$(TEST_PREFIX)/%: $(TEST_OBJ_PREFIX)/%.o $(O_FILES) | $(TEST_PREFIX)
	$(LD) $(O_FILES) $< -o $@


ifeq ($(COMPLETE_PREREQUISITES), true) #1

$(OBJ_PREFIX)/%.d: $(SOURCE_PREFIX)/%.c* | $(OBJ_PREFIX)
	@$(CC) $(INCLUDE) $(AUTO_GENERATE_FLAG) $< -MF $@ -MT "$@ $(OBJ_PREFIX)/$*.o"

include $(O_FILES:.o=.d) $(OBJ_PREFIX)/main.d

endif #1

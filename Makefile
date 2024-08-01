SOURCE_PREFIX = source/
TEST_SOURCE_PREFIX = tests/
HEADER_PREFIX = include/
OBJ_PREFIX = obj/
TEST_OBJ_PREFIX = obj/test/
BUILD_PREFIX = bin/Debug/
TEST_BUILD_PREFIX = bin/Tests/
DEFINES = -DDebug

COMMON_INCLUDE = -I $(HEADER_PREFIX)

CXX = g++
COMMON_FLAGS = -std=c++20 $(DEFINES) $(COMMON_INCLUDE)
FLAGS = $(COMMON_FLAGS) -g
RELEASE_FLAGS = $(COMMON_FLAGS) -O3
LINK_FLAGS = 
TEST_LINK_FLAGS = 

BIN_NAME = SMGNetMultiplayerServer
TEST_BIN_NAME = $(BIN_NAME)

objects = 
testObjects = 
main = main.o
testMain = main.o

OBJS = $(foreach obj, $(objects), $(OBJ_PREFIX)$(obj))
TEST_OBJS = $(foreach obj, $(testObjects), $(TEST_OBJ_PREFIX)$(obj))
MAIN = $(OBJ_PREFIX)$(main)
TEST_MAIN = $(TEST_OBJ_PREFIX)$(testMain)
DEPS = $(OBJS:.o=.d) $(MAIN:.o=.d)
TEST_DEPS = $(DEPS) $(TEST_OBJS:.o=.d) $(TEST_MAIN:.o=.d)

.PHONY: Debug Release cleanTest cleanDebug

SMGNetMultiplayerServer: $(DEPS) $(OBJS) $(MAIN)
	$(CXX) -o $(BUILD_PREFIX)$(BIN_NAME) $(OBJS) $(MAIN) $(LINK_FLAGS)

Test: $(TEST_DEPS) $(OBJS) $(TEST_OBJS) $(TEST_MAIN)
	$(CXX) -o $(TEST_BUILD_PREFIX)$(TEST_BIN_NAME) $(OBJS) $(TEST_OBJS) $(TEST_MAIN) $(TEST_LINK_FLAGS)

Debug: SMGNetMultiplayerServer

Release: SMGNetMultiplayerServer

$(OBJ_PREFIX)%.o:
	$(CXX) -c -o $@ $(SOURCE_PREFIX)$*.cpp $(FLAGS)

$(TEST_OBJ_PREFIX)%.o:
	$(CXX) -c -o $@ $(TEST_SOURCE_PREFIX)$*.cpp $(TEST_FLAGS)

$(OBJ_PREFIX)%.d: $(SOURCE_PREFIX)%.cpp
	$(CXX) $< -MM -MF $@ -MT '$(OBJ_PREFIX)$*.o $@' $(FLAGS)

$(TEST_OBJ_PREFIX)%.d: $(TEST_SOURCE_PREFIX)%.cpp
	$(CXX) $< -MM -MF $@ -MT '$(TEST_OBJ_PREFIX)$*.o $@' $(TEST_FLAGS)

include $(OBJS:.o=.d) $(MAIN:.o=.d)

include $(TEST_OBJS:.o=.d) $(TEST_MAIN:.o=.d)

cleanDebug:
	rm $(OBJ_PREFIX)/*.o $(OBJ_PREFIX)/*.d $(BUILD_PREFIX)/*

cleanTest:
	rm $(TEST_BUILD_PREFIX)/* $(TEST_OBJ_PREFIX)/*.o $(TEST_OBJ_PREFIX)/*.d

CC=clang++ 
TARGET = nprr
SRC = main.cpp
OBJ_FILES = $(patsubst %.cpp,%.o,$(SRC))

CFLAGS = -Wall -std=c++11 -fsanitize-undefined-trap-on-error -fsanitize=integer-divide-by-zero
DEBUG_FLAGS = -g -O0 -DDEBUG -ferror-limit=10
# WARNING_FLAGS = -Wextra
LINK_FLAGS = -lglpk

.PHONY: all clean 

# all: CFLAGS += $(DEBUG_FLAGS) 
# all: LINKFLAG += $(DEBUG_FLAGS) 

all:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(LINK_FLAGS) $(WARNING_FLAGS) $(OBJ_FILES) -o $(TARGET)

all: $(OBJ_FILES)

%.o: %.cpp
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM/
	rm -f $(OBJ_FILES)


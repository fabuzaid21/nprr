CC=clang++ 
TARGET = nprr
SRC = main.cpp
OBJ_FILES = $(patsubst %.cpp,%.o,$(SRC))

CFLAGS = -Wall -std=c++11 -fsanitize-undefined-trap-on-error -fsanitize=integer-divide-by-zero
DEBUG_FLAGS = -g -O0 -DDEBUG -ferror-limit=10
# WARNING_FLAGS = -Wextra
LINKCC = $(CC)
LINKFLAG = $(CFLAGS) $(LDFLAGS)

.PHONY: all clean 

# all: CFLAGS += $(DEBUG_FLAGS) 
# all: LINKFLAG += $(DEBUG_FLAGS) 
# all: $(OBJ_FILES)

all:
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) $(WARNING_FLAGS) $(SRC) -o $(TARGET)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
	rm -rf $(TARGET).dSYM/
	rm -f $(OBJ_FILES)


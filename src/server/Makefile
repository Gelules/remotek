CPPFLAGS = -D_POSIX_C_SOURCE=200112L
CFLAGS = -std=c99 -pedantic -Wall -Wextra -Wvla -Werror
OBJ = server.o
BIN = $(OBJ:.o=)

all: $(BIN)

$(BIN): $(OBJ)

clean:
	$(RM) $(OBJ) $(BIN)

CC := gcc
CFLAGS := -Wall -O0 -g -Iinclude

EXE0 := exe/regalws
EXE1 := exe/regalws_client_sample

SRC0 := regalws.c regalws_main.c regalws_sample.c
SRC1 := regalws_client_sample.c

OBJ0 := $(SRC0:%.c=obj/%.o)
OBJ1 := $(SRC1:%.c=obj/%.o)

all: $(EXE0) $(EXE1)

$(EXE0): $(OBJ0)
	@mkdir -p exe
	$(CC) $(CFLAGS) -o $@ $(OBJ0) -luriparser -lhttp_parser

$(EXE1): $(OBJ1)
	@mkdir -p exe
	$(CC) $(CFLAGS) -o $@ $(OBJ1) -lhttp_parser

obj/%.o: src/%.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(EXE0) $(OBJ0)
	rm -f $(EXE1) $(OBJ1)
	rmdir obj
	rmdir exe

run: all
	./$(EXE0) 13111

new: clean all

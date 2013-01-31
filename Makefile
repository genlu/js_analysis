INCLUDES = 
LIBDIRS = 

CFILES = array_list.c al_helper.c read.c instr.c stack_sim.c writeSet.c slicing.c cfg.c function.c opcode.c syntax.c df.c main.c
HEADERS = array_list.h al_helper.h read.h instr.h stack_sim.h writeSet.h slicing.h cfg.h function.h opcode.h syntax.h df.h
OFILES = $(patsubst %.c, %.o, $(CFILES)) 

CC = gcc
#PROFILEFLAGS = -pg
PROFILEFLAGS =
CFLAGS = $(PROFILEFLAGS) -Wall -ggdb
LDFLAGS = $(PROFILEFLAGS)

LIBS =

all:	jsa

jsa:	$(OFILES) 
	$(CC) $(LDFLAGS) $(LIBDIRS) -o $@ $^ $(LIBS)

%.o:	%.c 
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o jsa
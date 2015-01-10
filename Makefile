CC= cc
CFLAGS= -O -DM_TERMINFO -UM_TERMCAP
LDFLAGS= -O
OBJS= fire.o

#Unix
# LDLIBS= -lcurses -lm

#Xenix
LDLIBS= -ltinfo -lm

fire: $(OBJS)
	$(CC) $(LDFLAGS) fire.o -o fire $(LDLIBS)

clean:
	rm -f $(OBJS) fire

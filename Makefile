CC := cc
CFLAGS := -O2
LDFLAGS := -O
OBJS := fire.o
LDLIBS := -lcurses -lm

fire: $(OBJS)
	$(CC) $(LDFLAGS) fire.o -o fire $(LDLIBS)

clean:
	rm -f $(OBJS) fire

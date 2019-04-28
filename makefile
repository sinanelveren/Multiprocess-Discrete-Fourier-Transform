CC = gcc
DB = gdb
CFLAGS = -o
DFLAGS = -g
MAIN = 111044074_main
PROGNAME = multiprocess_DFT

all:
	$(CC) -std=c11 -lm -c $(MAIN).c
	$(CC) $(MAIN).o -lm $(CFLAGS) $(PROGNAME)

debug:
	$(CC) -std=c11 $(DFLAGS) -lm $(MAIN).c $(CFLAGS) $(PROGNAME)
	$(DB) ./$(PROGNAME)


clean:
	rm -f $(PROGNAME) *.o *.dat *.txt *.log

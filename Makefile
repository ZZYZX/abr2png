CC         = g++
CFLAGS     = -Wall -pedantic
CFLAGS_OPT = -O2
LIBS       = -lstdc++ -lm -lpng -lz

all:
	$(CC) $(CFLAGS) $(CFLAGS_OPT) -c abr_util.cpp -s $(LIBS)
	$(CC) $(CFLAGS) $(CFLAGS_OPT) -c PngWrite.cpp -s $(LIBS)
	$(CC) $(CFLAGS) $(CFLAGS_OPT) -c abr.cpp -s $(LIBS)
	$(CC) abr.o abr_util.o PngWrite.o -o abr2png -s $(LIBS)

.PHONY: clean

clean:
	rm -rf abr2png *.o

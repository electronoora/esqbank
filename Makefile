#
# Makefile for esqbank
#
all: esqbank

esqbank: esqbank.c
	gcc -Wall -o esqbank esqbank.c -framework CoreMIDI -framework CoreFoundation

clean:
	rm -f *~ *.o esqbank


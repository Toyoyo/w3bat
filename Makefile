WRC=wrc
CC=wcl
CFLAGS=-l=windows -bt=windows -fe=w3bat.exe -ot
LIBS=ole2.lib

all:
	$(WRC) -q -zm -bt=nt -r -fo=w3bat.res w3bat.rc
	$(CC) $(CFLAGS) $(LIBS) w3bat.c w3bat.res

clean:
	rm -f w3bat.exe
	rm -f w3bat.o
	rm -f w3bat.res

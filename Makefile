files=Loader.c Quantifier.c Loader.h

build: initiator
initiator: $(files)
	gcc -o analyze $(files) -lm


package:
	tar -cvzf Gavin-Parrott-HW4.tar *.c *.h *.csv *.txt Makefile

clean:
	rm -f analyze
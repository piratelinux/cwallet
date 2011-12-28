all: cwallet.c
	gcc -o cwallet cwallet.c -ldb -lcrypto
clean:
	rm -f cwallet *~

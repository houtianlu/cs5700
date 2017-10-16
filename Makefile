all: check
clean:
	rm -rf *.o
	rm -rf sender
	rm -rf reciever
reciever.o: reciever.c
	gcc -c -Wall -Werror -fpic -D_GNU_SOURCE -std=c99 -o reciever.o reciever.c
sender.o: sender.c
	gcc -c -Wall -Werror -fpic -D_GNU_SOURCE -std=c99 -o sender.o sender.c
sender: sender.o util.o
	gcc -g -o sender sender.o util.o
reciever: reciever.o util.o
	gcc -g -o reciever reciever.o util.o
util.o: util.c
	gcc -c -Wall -Werror -fpic -o util.o util.c
check: sender reciever
buildSender: sender
	./sender -m 1 -p 15055 -h localhost -f /Users/tianluhou/Desktop/testFile
buildReciever: reciever
	./reciever -m 1 -p 16055 -h localhost
dist:
	dir=`basename $$PWD`; cd ..; tar cvf $$dir.tar ./$$dir; gzip $$dir.tar

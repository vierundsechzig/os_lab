OBJS_MAIN = main.o commands.o fs.o disk.o
OBJS_LONGFILE = longfiletest.o commands.o fs.o disk.o

all: main longfile

main: $(OBJS_MAIN)
	gcc $(OBJS_MAIN) -o main
longfile: $(OBJS_LONGFILE)
	gcc $(OBJS_LONGFILE) -o longfile
main.o: main.c fs.h disk.h
	gcc -c main.c -o main.o
longfiletest.o: longfiletest.c fs.h disk.h
	gcc -c longfiletest.c -o longfiletest.o
commands.o: commands.c fs.h disk.h
	gcc -c commands.c -o commands.o
fs.o: fs.c fs.h disk.h
	gcc -c fs.c -o fs.o
disk.o: disk.c disk.h
	gcc -c disk.c -o disk.o
clean:
	rm -rf *.o main

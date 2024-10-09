LANGUAGE=FRENCH
all: hello-world international-hello-world fancy-hello-world

hello-world:
	gcc -c hello-world.c
	gcc -o hello-world hello-world.o

international-hello-world:
	gcc -c -Wall -g international-hello-world.c -D$(LANGUAGE)
	gcc -g -o international-hello-world international-hello-world.o

fancy-hello-world:
	gcc -c fancy-hello-world.c
	gcc -o fancy-hello-world fancy-hello-world.o

clean:
	rm -f hello-world.o hello-world.exe hello-world
	rm -f international-hello-world.o international-hello-world.exe international-hello-world
	rm -f fancy-hello-world.o fancy-hello-world.exe fancy-hello-world
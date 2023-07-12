all: test example

test: bigpacks-test.c bigpacks.c bigpacks.h
		gcc -o bigpacks-test bigpacks-test.c bigpacks.c -I.

example: bigpacks-example.c bigpacks.c bigpacks.h
		gcc -o bigpacks-example bigpacks-example.c bigpacks.c -I.

clean:
		rm -f bigpacks-test bigpacks-example

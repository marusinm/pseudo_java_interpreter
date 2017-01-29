CC=gcc
CFLAGS=-c -pedantic -Wall

all: prekladac

prekladac: main.o scanner.o ial.o ilist.o parser.o precedence.o stack.o stack_test.o str.o garb.o interpret.o frame.o built_in_functions.o
	$(CC) main.o scanner.o ial.o ilist.o parser.o precedence.o stack.o stack_test.o str.o garb.o interpret.o frame.o built_in_functions.o -o main

main.o: main.c; $(CC) $(CFLAGS) main.c
scanner.o: scanner.c; $(CC) $(CFLAGS) scanner.c
ial.o: ial.c; $(CC) $(CFLAGS) ial.c
ilist.o: ilist.c; $(CC) $(CFLAGS) ilist.c
parser.o: parser.c; $(CC) $(CFLAGS) parser.c
precedence.o: precedence.c; $(CC) $(CFLAGS) precedence.c
stack.o: stack.c; $(CC) $(CFLAGS) stack.c
stack_test.o: stack_test.c; $(CC) $(CFLAGS) stack_test.c
str.o: str.c; $(CC) $(CFLAGS) str.c
garb.o: garb.c; $(CC) $(CFLAGS) garb.c
interpret.o: interpret.c; $(CC) $(CFLAGS) interpret.c
frame.o: frame.c; $(CC) $(CFLAGS) frame.c
built_in_functions.o: built_in_functions.c; $(CC) $(CFLAGS) built_in_functions.c

clean:
	rm -f *.o *.out main scanner ial ilist parser precedence stack stack_test str garb built_in_functions

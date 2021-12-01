CC      = gcc
CFLAGS  = -g
RM      = rm -f
AR						= ar

main: soucin.c podil.c soucet.c
	$(CC) $(CFLAGS) -c $^
	$(AR) -rc osy_lib.a *.o
	$(CC) $(CFLAGS) -o $@ $@.c osy_lib.a
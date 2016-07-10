CPROG=		myftpc
DPROG=		myftpd
COBJS=		myftpc.o func.o
DOBJS=		myftpd.o func.o
CC=			gcc

PROGS=		$(CPROG) $(DPROG)

CFLAGS=	-O -DDEBUG

all: $(CPROG) $(DPROG)

$(CPROG): $(COBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(CPROG) $(COBJS)

$(DPROG): $(DOBJS) $(LIBS)
	$(CC) $(CFLAGS) -o $(DPROG) $(DOBJS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $*.c

myftpc.o: def.h
myftpd.o: def.h
func.o: def.h

clean:
	rm -f $(PROGS) *.o core *.core *.bak *_r *~

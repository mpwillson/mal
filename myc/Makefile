TARGET=step7_quote
S_OBJS=step7_quote.o reader.o printer.o env.o core.o
CFLAGS=-g -Wno-pointer-to-int-cast
LDFLAGS=-lreadline

${TARGET}: $(S_OBJS) mal.h reader.h printer.h env.h core.h
	cc ${CFLAGS} -o $@ ${S_OBJS} ${LDFLAGS}

${S_OBJS}: mal.h

clean:
	rm -f ${S_OBJS} ${TARGET} ${TARGET}.exe

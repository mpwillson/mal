TARGET=step9_try
S_OBJS=${TARGET}.o reader.o printer.o env.o core.o mem.o
CFLAGS=-g -Wall -Wno-pointer-to-int-cast
LDFLAGS=-lreadline

${TARGET}: $(S_OBJS) mal.h reader.h printer.h env.h core.h mem.h
	cc ${CFLAGS} -o $@ ${S_OBJS} ${LDFLAGS}

${S_OBJS}: mal.h

clean:
	rm -f ${S_OBJS} ${TARGET} ${TARGET}.exe

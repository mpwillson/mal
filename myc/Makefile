#TARGET=step1_read_print
#S_OBJS=step1_read_print.o reader.o printer.o
TARGET=step6_file
S_OBJS=step6_file.o reader.o printer.o env.o core.o
CFLAGS=-g -Wno-pointer-to-int-cast
LDFLAGS=-lreadline

${TARGET}: $(S_OBJS) mal.h reader.h printer.h env.h
	cc ${CFLAGS} -o $@ ${S_OBJS} ${LDFLAGS}

${S_OBJS}: mal.h

clean:
	rm -f ${S_OBJS} ${TARGET} ${TARGET}.exe

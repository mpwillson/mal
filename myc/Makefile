#TARGET=step1_read_print
#S_OBJS=step1_read_print.o reader.o printer.o
TARGET=step4_if_fn_do
S_OBJS=step4_if_fn_do.o reader.o printer.o env.o
CFLAGS=-g -Wno-pointer-to-int-cast

${TARGET}: $(S_OBJS) mal.h reader.h printer.h env.h
	cc ${CFLAGS} -o $@ ${S_OBJS}

${S_OBJS}: mal.h

clean:
	rm -f ${S_OBJS} ${TARGET} ${TARGET}.exe

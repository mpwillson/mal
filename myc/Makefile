#TARGET=step1_read_print
#S_OBJS=step1_read_print.o reader.o printer.o
TARGET=step3_env
S_OBJS=step3_env.o reader.o printer.o env.o
CFLAGS=-g

${TARGET}: $(S_OBJS) mal.h reader.h printer.h env.h
	cc ${CFLAGS} -o $@ ${S_OBJS}

clean:
	rm -f ${S_OBJS} ${TARGET} ${TARGET}.exe

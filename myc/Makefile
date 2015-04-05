TARGET=step1_read_print
S1_OBJS=step1_read_print.o reader.o printer.o

${TARGET}: $(S1_OBJS) reader.h printer.h
	cc -g -o $@ $^

clean:
	rm -f ${S1_OBJS} ${TARGET}

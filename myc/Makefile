TARGET=step1_read_print
S1_OBJS=step1_read_print.o reader.o symtab.o printer.o

${TARGET}: $(S1_OBJS)
	cc -g -o $@ $^

clean:
	rm -f ${S1_OBJS} ${TARGET}

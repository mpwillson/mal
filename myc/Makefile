#TARGET=step1_read_print
#S_OBJS=step1_read_print.o reader.o printer.o
TARGET=step2_eval
S_OBJS=step2_eval.o reader.o printer.o

${TARGET}: $(S_OBJS) 
	cc -g -o $@ $^

clean:
	rm -f ${S_OBJS} ${TARGET}

#
# mouse:~ppr/src/filters_typeset/pod/Makefile
#

LEX = flex
YACC = yacc
CC = gcc -DYYDEBUG=1

OBJS=pod_parser.o pod_lexer.o

INTERMEDIATES=pod_parser.c pod_parser.h pod_lexer.c

PROGS=pod2common

pod2common: $(OBJS)
	$(CC) -o pod2common $(OBJS)

pod_parser.o: pod_parser.c

pod_lexer.o: pod_lexer.c pod_parser.h

pod_parser.c pod_parser.h: pod.y
	$(YACC) -d pod.y
	mv y.tab.c pod_parser.c
	mv y.tab.h pod_parser.h

pod_lexer.c: pod.l
	$(LEX) -t pod.l >pod_lexer.c

clean:
	rm -f $(INTERMEDIATES) $(OBJS) $(PROGS) *~

# end of file


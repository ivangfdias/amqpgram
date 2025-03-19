CC=gcc
CFLAGS=
GLIB=`pkg-config --cflags glib-2.0`
AR=ar
ARFLAGS= -rcv
DOC=doxygen

VPATH=fuzzer/
PROGRAM=main.c
LIB= AMQP.a libpacket.a
OBJS= AMQP.o Packet.o utils.o grammar-parser.o packet-generator.o Connection.o Grammar_Interface.o

#AMQP.o 
#Consumer.o Basic.o Queue.o Channel.o Connection.o 


##################################
.SUFFIXES: .c .o .a

all: generator.o amqpgram 
	
.c.o:
	${CC} ${CFLAGS} ${GLIB} -c $<

generator.o:
	${CC} ${CFLAGS} ${GLIB} -c grammar/packet-generator.c grammar/grammar-parser.c -lglib-2.0


${LIB}: ${OBJS} ${generator.o}
	${AR} ${ARFLAGS} $@ $< ${OBJS}

amqpgram: ${PROGRAM} ${OBJS} ${LIB} ${generator.o}
	${CC} ${CFLAGS} ${GLIB} -o  $@ $< ${LIB} -pthread -lglib-2.0


clean:
	@rm -rf *.o html *.a amqpgram logs/*


test-5m-r0-p0:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 0 -P 0
test-5m-r1-p0:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 1 -P 0
test-5m-r2-p0:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 2 -P 0
test-5m-r3-p0:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 3 -P 0
test-5m-r4-p0:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 4 -P 0


test-5m-r0-p1:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 0 -P 1
test-5m-r0-p2:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 0 -P 2
test-5m-r0-p3:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 0 -P 3
test-5m-r0-p4:
	./monitor.sh ./amqpgram 127.0.0.1 -t 300 -R 0 -P 4


LOGIN = anmittal
ID = 109176039
CC = gcc



LIBS =  /home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2 -I/home/courses/cse533/Stevens/unpv13e/lib

all: client_${LOGIN}  server_${LOGIN} ODR_${LOGIN} prhwaddrs
	
prhwaddrs: prhwaddrs.o get_hw_addrs.o 
	${CC} -o prhwaddrs prhwaddrs.o get_hw_addrs.o ${LIBS}

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

prhwaddrs.o: prhwaddrs.c
	${CC} ${FLAGS} -c prhwaddrs.c

client.o : client.c common.h
	${CC} ${FLAGS} -c client.c

server.o : server.c common.h
	${CC} ${FLAGS} -c server.c

client_${LOGIN} : client.o common.o
	${CC} -o $@ client.o ${LIBS}

server_${LOGIN} : server.o common.o
	${CC} -o $@ server.o ${LIBS}

odr.o : odr.c common.h
	${CC} ${FLAGS} -DPROTO=${ID} -c odr.c

common.o : common.c common.h
	${CC} ${FLAGS} -DPROTO=${ID} -c common.c

ODR_${LOGIN} : odr.o get_hw_addrs.o
	${CC} -o $@ odr.o get_hw_addrs.o ${LIBS}
clean:
	rm prhwaddrs *.o client_${LOGIN} server_${LOGIN} ODR_${LOGIN}


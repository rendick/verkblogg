CC = gcc
CFLAGS = -Wall -Wextra

TARGET = verkblogg

all: ${TARGET}

${TARGET}: ${TARGET}.c
	${CC} ${CFLAGS} -o ${TARGET} ${TARGET}.c

clean:
	rm ${TARGET}



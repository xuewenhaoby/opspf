INC = ./include
LIB = ./library
OBJ = ./obj
SRC = ./src
BIN = ./bin

INC_LOG4CPLUS = /home/fnl/Software/log4cplus/include
LIB_LOG4CPLUS = /home/fnl/Software/log4cplus/lib
INC_PYTHON = /usr/include/python2.7

TARGET = main
BIN_TARGET = ${BIN}/${TARGET}

CC = g++
CFLAGS = -g -Wall -std=c++11 -fno-stack-protector -I${INC} 
SO = -lpthread
#####################version 3#############################
#################use liblog4cplus.so#######################

SOURCE = $(wildcard ${SRC}/*.cpp)

${BIN_TARGET}:${SOURCE} 
	$(CC) $(CFLAGS) -o $@ $^ ${SO}

.PHONY:clean
clean:
	rm -rf ${BIN_TARGET} 

#####################version 2#############################

# SOURCE = $(wildcard ${SRC}/*.cpp)
# OBJECT = $(patsubst %.cpp,${OBJ}/%.o,$(notdir ${SOURCE}))

# ${BIN_TARGET}:${OBJECT}
# 	$(CC) -o $@ ${OBJECT}

# ${OBJ}/%.o:${SRC}/%.cpp
# 	$(CC) $(CFLAGS) -o $@ -c $<

# .PHONY:clean
# clean:
# 	find ${OBJ} -name *.o -exec rm -rf {} \;
# 	rm -rf ${BIN_TARGET} 

#####################version 1#############################

# ${BIN_TARGET}:${OBJ}/main.o ${OBJ}/func.o
# 	$(CC) -o $@ $^

# ${OBJ}/main.o:${SRC}/main.cpp ${INC}/func.h
# 	$(CC) $(CFLAGS) -o $@ -c $<

# ${OBJ}/func.o:${SRC}/func.cpp ${INC}/func.h
# 	$(CC) $(CFLAGS) -o $@ -c $<

# clean:
# 	rm -rf ${OBJ}/*.o ${BIN_TARGET}

######################version 0############################

# bin/main:obj/main.o obj/func.o
# 	g++ -o bin/main obj/main.o obj/func.o

# obj/main.o:src/main.cpp include/func.h
# 	g++ -g -Wall -Iinclude -o obj/main.o -c src/main.cpp

# obj/func.o:src/func.cpp include/func.h
# 	g++ -g -Wall -Iinclude -o obj/func.o -c src/func.cpp

# clean:
# 	rm -rf obj/*.o bin/main

#####################static_lib#############################

# ${BIN}/main-a:${SRC}/main.cpp ${LIB}/libfunc.a 
# 	$(CC) $(CFLAGS) -o $@ $^

# ${LIB}/libfunc.a:${OBJ}/func.o
# 	ar crv $@ $^

# ${OBJ}/func.o:${SRC}/func.cpp ${INC}/func.h
# 	$(CC) $(CFLAGS) -o $@ -c $<

# clean:
# 	rm -rf ${LIB}/libfunc.a ${OBJ}/*.o ${BIN}/mian-a 

####################dynamic_lib##############################

# ${BIN}/main-so:${SRC}/main.cpp ${LIB}/libfunc.so
# 	$(CC) $(CFLAGS) -o $@ $< -L${LIB} -lfunc

# ${LIB}/libfunc.so:${OBJ}/func.o
# 	$(CC) -shared -o $@ $^

# ${OBJ}/func.o:${SRC}/func.cpp ${INC}/func.h
# 	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

# clean:
# 	rm -rf ${OBJ}/*.o ${LIB}/*.so ${BIN}/mian-so


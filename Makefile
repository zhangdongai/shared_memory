TARGET=sharedmemory

CC=g++

CPPFLAGS=-D__LINUX__ \
		-g 			 \
		-rdynamic    \
		-O0			 \
		-I.         \
		-I./test    \
		-std=c++11	 \

LINKFALGS=-lpthread

#CPPOBJS=$(shell find ./ -name "*.cpp" | awk -F".cpp" '{printf "%s.o\n", $$1}')
CPPOBJS=$(shell find ./ -name "*.cpp")

all:
	$(CC) $(CPPFLAGS) $(CPPOBJS) $(LINKFALGS) -o $(TARGET)

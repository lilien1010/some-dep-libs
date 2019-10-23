CC = gcc
CXX32 = g++
CC32 = gcc

ARRU = ar -ru

EXTRAFLAGS = -Wno-unused-local-typedefs -Wno-unknown-warning-option
CXXFLAG = -Wall -O0 -g $(PGOPT) -D_REENTRANT $(EXTRAFLAGS)
CXXFLAG_R = -Wall -O2 -O3 -D_REENTRANT 

#INCLUDE = 

SRC_COMM = TEACrypto.cpp


# -------------------------------------------------------------------------

#CXXFLAG_R = -Wall -O2 -O3 -D_REENTRANT $(INCLUDE) $(DEFINE_SEL) 

#OBJ_COMM  = $(SRC_COMM:.cpp=.o) $(SRC_COMM:.cc=.o) $(SRC_COMM:.c=.o)
OBJ_COMM  = $(SRC_COMM:.c=.o)
OBJ_COMM_R  = $(SRC_COMM:.cpp=.ro) $(SRC_COMM:.cc=.ro) $(SRC_COMM:.c=.ro)

.SUFFIXES: .o .cpp
.cpp.o:
	$(CXX) $(CXXFLAG) ${INCLUDE} -c -o $@ $<

.SUFFIXES: .o .cc
.cc.o:
	$(CXX) $(CXXFLAG) ${INCLUDE} -c -o $@ $<

.SUFFIXES: .o .c
.c.o:
	$(CC) $(CXXFLAG) ${INCLUDE} -c -o $@ $<

.SUFFIXES: .ro .cpp
.cpp.ro:
	$(CXX32) $(CXXFLAG_R) ${INCLUDE} -c -o $@ $<

.SUFFIXES: .ro .cc
.cc.ro:
	$(CXX32) $(CXXFLAG_R) ${INCLUDE} -c -o $@ $<

.SUFFIXES: .ro .c
.c.ro:
	$(CC) $(CXXFLAG_R) ${INCLUDE} -c -o $@ $<

all: libteacrypto.a #libteacrypto.ra

release: libteacrypto.ra

libteacrypto.a: $(OBJ_COMM) | ../../lib
	$(ARRU) libteacrypto.a $(OBJ_COMM)
#	ranlib libteacrypto.a

../../lib:
	mkdir ../../lib
  
libteacrypto.ra: $(OBJ_COMM_R)
	$(ARRU) ../../lib/libteacrypto.ra $(OBJ_COMM_R)
#	ranlib libteacrypto.ra

#test: libteacrypto.a main.cpp
#	$(CXX) $(CXXFLAG) -o test main.cpp libteacrypto.a

depend:
	mkdep $(CXXFLAG) $(SRC_COMM)

install:
	
.PHONY : clean
clean: 
	rm -f *.o
	rm -f *.ro
	rm -f ../../lib/libteacrypto.a


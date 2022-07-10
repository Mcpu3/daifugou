# Makefile 
CC	     = gcc -std=c99
CXX	     = g++ -std=c++11

CFLAGS   = -Wall -g
CXXFLAGS = -Wall -g

HEADERS = card.h cardset.h dealer.h gamestatus.h player.h \
          simpleplayer.h defaultplayer.h thinkTA1.h group5.h
OBJS = dealer.o player.o simpleplayer.o defaultplayer.o group5.o

.SUFFIXES: .o .cc .cpp .c++ .c

.c.o: $(HEADERS)
	$(CC) -c $(CFLAGS) $<

.cc.o: $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $<

.cpp.o: $(HEADERS)
	$(CXX) -c $(CXXFLAGS) $< 

all: daifugou
	@echo "build finished."

daifugou: daifugou.o $(OBJS) $(HEADERS) libplayingcards.a
	$(CXX) daifugou.o $(OBJS) -o daifugou -L. -lplayingcards -lpthread

lib: libplayingcards.a

#libplayingcards.a: card.o cardset.o thinkTA1.o
#	ar rcs libplayingcards.a card.o cardset.o thinkTA1.o

depend:
	makedepend *.cpp

clean   :
	rm -f core daifugou *.o *~
#	rm -f core daifugou `echo *.o | sed -e 's/ThinkTA1.o//'` *~

# DO NOT DELETE

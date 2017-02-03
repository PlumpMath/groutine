CXX=g++
WARNING=-Wall -Werror
CXXFLAGS=-O2 -I../
OBJECTS=groutine.o

all: $(OBJECTS)
	$(CXX) $(OBJECTS) example_alice_and_bob.cpp -o example_alice_and_bob $(CXXFLAGS)

groutine.o: groutine.cpp groutine.h
	$(CXX) -c groutine.cpp $(CXXFLAGS)

clean:
	rm -fv *.o example_alice_and_bob

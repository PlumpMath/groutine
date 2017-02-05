CXX=g++
WARNING=-Wall -Werror
CXXFLAGS=-O2 -I../
OBJECTS=example_alice_and_bob

all: $(OBJECTS)

example_alice_and_bob: groutine.o example_alice_and_bob.cpp
	$(CXX) groutine.o example_alice_and_bob.cpp -o example_alice_and_bob $(CXXFLAGS)

groutine.o: groutine.cpp groutine.h
	$(CXX) -c groutine.cpp $(CXXFLAGS)

clean:
	rm -fv *.o example_alice_and_bob

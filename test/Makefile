CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -I../Include -I../src
LIBS = -lssl -lcrypto -lsodium -lpthread

TEST_SOURCES = test_crypto.cpp test_stego.cpp
SRC_SOURCES = ../src/crypto_utils.cpp ../src/image_gen.cpp ../src/stego.cpp ../src/image_utils.cpp ../Include/lodepng.cpp
CATCH_SRC = catch_amalgamated.cpp

test_runner: $(TEST_SOURCES) $(SRC_SOURCES) $(CATCH_SRC)
	$(CXX) $(CXXFLAGS) -o test_runner \
		$(TEST_SOURCES) $(SRC_SOURCES) $(CATCH_SRC) $(LIBS)

.PHONY: test clean
test: test_runner
	./test_runner --allow-running-no-tests

clean:
	rm -f test_runner *.o

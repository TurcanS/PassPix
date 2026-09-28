CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -I../Include -I../src $(EXTRA_CXXFLAGS)
LIBS = -lsodium -lpthread

# Catch2's test macros use __COUNTER__, which newer clang flags under -pedantic
ifneq (,$(findstring clang,$(shell $(CXX) --version)))
    CXXFLAGS += -Wno-c2y-extensions -Wno-unknown-warning-option
endif

ifeq ($(OS),Windows_NT)
    LIBS = -lsodium -ladvapi32
endif

TEST_SOURCES = test_crypto.cpp test_stego.cpp
SRC_SOURCES = ../src/crypto_utils.cpp ../src/image_gen.cpp ../src/stego.cpp ../src/image_utils.cpp ../Include/lodepng.cpp
HEADERS = $(wildcard ../src/*.h) ../Include/lodepng.h
CATCH_SRC = catch_amalgamated.cpp

test_runner: $(TEST_SOURCES) $(SRC_SOURCES) $(HEADERS) $(CATCH_SRC)
	$(CXX) $(CXXFLAGS) -o test_runner \
		$(TEST_SOURCES) $(SRC_SOURCES) $(CATCH_SRC) $(LIBS)

.PHONY: test clean
test: test_runner
	./test_runner --allow-running-no-tests

clean:
	rm -f test_runner test_runner.exe *.o

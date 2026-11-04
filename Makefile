CXX ?= g++-16
CXXFLAGS ?= -std=c++23 -Wall -Wextra -O2 -Isrc

all: mltt_vectors automath_logic

mltt_vectors: examples/mltt_vectors.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/mltt_vectors.cpp -o mltt_vectors

automath_logic: examples/automath_logic.cpp src/automath.hpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/automath_logic.cpp -o automath_logic

test: all
	@echo "Running MLTT vectors example..."
	./mltt_vectors
	@echo "\nRunning Automath logic example..."
	./automath_logic

clean:
	rm -f mltt_vectors automath_logic

.PHONY: all test clean

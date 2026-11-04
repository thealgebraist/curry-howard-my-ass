CXX ?= g++-16
CXXFLAGS ?= -std=c++23 -Wall -Wextra -O2 -Isrc

all: test_suite mltt_vectors automath_logic inverse_galois

test_suite: tests/test_suite.cpp src/lambda_pi.hpp src/automath.hpp
	$(CXX) $(CXXFLAGS) tests/test_suite.cpp -o test_suite

mltt_vectors: examples/mltt_vectors.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/mltt_vectors.cpp -o mltt_vectors

automath_logic: examples/automath_logic.cpp src/automath.hpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/automath_logic.cpp -o automath_logic

inverse_galois: examples/inverse_galois.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/inverse_galois.cpp -o inverse_galois

test: all
	@echo "\n=== Running Full λN / λP / Automath Test Suite ==="
	./test_suite
	@echo "\n=== Running MLTT vectors example ==="
	./mltt_vectors
	@echo "\n=== Running Automath logic example ==="
	./automath_logic
	@echo "\n=== Running Inverse Galois Proof Checker ==="
	./inverse_galois

clean:
	rm -f test_suite mltt_vectors automath_logic inverse_galois

.PHONY: all test clean

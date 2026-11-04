CXX ?= g++-16
CXXFLAGS ?= -std=c++23 -Wall -Wextra -O2 -Isrc

all: test_suite mltt_vectors automath_logic inverse_galois scheme_interpreter cpp_template_evaluator blink_rendering_model blink_parser_lifecycle blink_parser_bisimulation blink_tree_builder_p_start_bisimulation blink_css_cascade_bisimulation blink_css_selector_bisimulation blink_css_combinator_bisimulation

test_suite: tests/test_suite.cpp src/lambda_pi.hpp src/automath.hpp
	$(CXX) $(CXXFLAGS) tests/test_suite.cpp -o test_suite

mltt_vectors: examples/mltt_vectors.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/mltt_vectors.cpp -o mltt_vectors

automath_logic: examples/automath_logic.cpp src/automath.hpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/automath_logic.cpp -o automath_logic

inverse_galois: examples/inverse_galois.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/inverse_galois.cpp -o inverse_galois

scheme_interpreter: examples/scheme_interpreter.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/scheme_interpreter.cpp -o scheme_interpreter

cpp_template_evaluator: examples/cpp_template_evaluator.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/cpp_template_evaluator.cpp -o cpp_template_evaluator

blink_rendering_model: examples/blink_rendering_model.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_rendering_model.cpp -o blink_rendering_model

blink_parser_lifecycle: examples/blink_parser_lifecycle.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_parser_lifecycle.cpp -o blink_parser_lifecycle

blink_parser_bisimulation: examples/blink_parser_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_parser_bisimulation.cpp -o blink_parser_bisimulation

blink_tree_builder_p_start_bisimulation: examples/blink_tree_builder_p_start_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tree_builder_p_start_bisimulation.cpp -o blink_tree_builder_p_start_bisimulation

blink_css_cascade_bisimulation: examples/blink_css_cascade_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_cascade_bisimulation.cpp -o blink_css_cascade_bisimulation

blink_css_selector_bisimulation: examples/blink_css_selector_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_selector_bisimulation.cpp -o blink_css_selector_bisimulation

blink_css_combinator_bisimulation: examples/blink_css_combinator_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_combinator_bisimulation.cpp -o blink_css_combinator_bisimulation

test: all
	@echo "\n=== Running Full λN / λP / Automath Test Suite ==="
	./test_suite
	@echo "\n=== Running MLTT vectors example ==="
	./mltt_vectors
	@echo "\n=== Running Automath logic example ==="
	./automath_logic
	@echo "\n=== Running Inverse Galois Proof Checker ==="
	./inverse_galois
	@echo "\n=== Running typed Scheme evaluator in λΠ ==="
	./scheme_interpreter
	@echo "\n=== Running C++ template evaluator in λΠ ==="
	./cpp_template_evaluator
	@echo "\n=== Running λΠ Blink rendering model ==="
	./blink_rendering_model
	@echo "\n=== Running λΠ Blink parser lifecycle model ==="
	./blink_parser_lifecycle
	@echo "\n=== Running finite λΠ Blink parser bisimulation ==="
	./blink_parser_bisimulation
	@echo "\n=== Running finite λΠ Blink tree-builder p-start bisimulation ==="
	./blink_tree_builder_p_start_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS cascade bisimulation ==="
	./blink_css_cascade_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS selector bisimulation ==="
	./blink_css_selector_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS combinator bisimulation ==="
	./blink_css_combinator_bisimulation

clean:
	rm -f test_suite mltt_vectors automath_logic inverse_galois scheme_interpreter cpp_template_evaluator blink_rendering_model blink_parser_lifecycle blink_parser_bisimulation blink_tree_builder_p_start_bisimulation blink_css_cascade_bisimulation blink_css_selector_bisimulation blink_css_combinator_bisimulation

.PHONY: all test clean

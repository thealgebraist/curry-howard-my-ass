CXX ?= g++-16
CXXFLAGS ?= -std=c++23 -Wall -Wextra -O2 -Isrc

all: test_suite mltt_vectors automath_logic inverse_galois scheme_interpreter cpp_template_evaluator blink_rendering_model blink_parser_lifecycle blink_parser_bisimulation blink_tree_builder_p_start_bisimulation blink_tree_builder_p_end_bisimulation blink_tree_builder_li_end_bisimulation blink_tree_builder_block_end_bisimulation blink_css_cascade_bisimulation blink_css_selector_bisimulation blink_css_combinator_bisimulation blink_css_match_winner blink_css_selector_color blink_css_revert_layer blink_css_priority_layer_normalization blink_css_cascade_priority blink_css_cascade_map blink_css_revert_color blink_css_revert_origin blink_css_revert_origin_color blink_css_revert_origin_full_priority blink_css_color_defaulting blink_style_recalc_gate blink_style_recalc_dependencies blink_style_recalc_children blink_style_recalc_traversal blink_style_recalc_copy blink_computed_style_difference blink_dom_insert_html_element blink_tree_builder_p_start_dom_insert blink_tokenized_p_start_dom blink_layout_dirty_bisimulation blink_tokenizer_open_tag_bisimulation blink_tokenizer_tag_name_bisimulation blink_tokenizer_attribute_name_bisimulation blink_tokenizer_attribute_value_bisimulation blink_numeric_html_entity_bisimulation blink_html_named_entity blink_html_named_entity_states blink_html_named_entity_remainder blink_segmented_entity_source_bisimulation blink_named_entity_segmented_composition blink_process_entity_bisimulation blink_tokenizer_attribute_entity_bisimulation blink_paint_chunker_bisimulation blink_pending_text_bisimulation blink_dom_text_flush_bisimulation

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

blink_tree_builder_p_end_bisimulation: examples/blink_tree_builder_p_end_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tree_builder_p_end_bisimulation.cpp -o blink_tree_builder_p_end_bisimulation

blink_tree_builder_li_end_bisimulation: examples/blink_tree_builder_li_end_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tree_builder_li_end_bisimulation.cpp -o blink_tree_builder_li_end_bisimulation

blink_tree_builder_block_end_bisimulation: examples/blink_tree_builder_block_end_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tree_builder_block_end_bisimulation.cpp -o blink_tree_builder_block_end_bisimulation

blink_css_cascade_bisimulation: examples/blink_css_cascade_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_cascade_bisimulation.cpp -o blink_css_cascade_bisimulation

blink_css_selector_bisimulation: examples/blink_css_selector_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_selector_bisimulation.cpp -o blink_css_selector_bisimulation

blink_css_combinator_bisimulation: examples/blink_css_combinator_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_combinator_bisimulation.cpp -o blink_css_combinator_bisimulation

blink_css_match_winner: examples/blink_css_match_winner.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_match_winner.cpp -o blink_css_match_winner

blink_css_selector_color: examples/blink_css_selector_color.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_selector_color.cpp -o blink_css_selector_color

blink_css_revert_layer: examples/blink_css_revert_layer.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_revert_layer.cpp -o blink_css_revert_layer

blink_css_priority_layer_normalization: examples/blink_css_priority_layer_normalization.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_priority_layer_normalization.cpp -o blink_css_priority_layer_normalization

blink_css_cascade_priority: examples/blink_css_cascade_priority.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_cascade_priority.cpp -o blink_css_cascade_priority

blink_css_cascade_map: examples/blink_css_cascade_map.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_cascade_map.cpp -o blink_css_cascade_map

blink_css_revert_color: examples/blink_css_revert_color.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_revert_color.cpp -o blink_css_revert_color

blink_css_revert_origin: examples/blink_css_revert_origin.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_revert_origin.cpp -o blink_css_revert_origin

blink_css_revert_origin_color: examples/blink_css_revert_origin_color.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_revert_origin_color.cpp -o blink_css_revert_origin_color

blink_css_revert_origin_full_priority: examples/blink_css_revert_origin_full_priority.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_revert_origin_full_priority.cpp -o blink_css_revert_origin_full_priority

blink_css_color_defaulting: examples/blink_css_color_defaulting.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_css_color_defaulting.cpp -o blink_css_color_defaulting

blink_style_recalc_gate: examples/blink_style_recalc_gate.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_style_recalc_gate.cpp -o blink_style_recalc_gate

blink_style_recalc_dependencies: examples/blink_style_recalc_dependencies.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_style_recalc_dependencies.cpp -o blink_style_recalc_dependencies

blink_style_recalc_children: examples/blink_style_recalc_children.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_style_recalc_children.cpp -o blink_style_recalc_children

blink_style_recalc_traversal: examples/blink_style_recalc_traversal.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_style_recalc_traversal.cpp -o blink_style_recalc_traversal

blink_style_recalc_copy: examples/blink_style_recalc_copy.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_style_recalc_copy.cpp -o blink_style_recalc_copy

blink_computed_style_difference: examples/blink_computed_style_difference.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_computed_style_difference.cpp -o blink_computed_style_difference

blink_layout_dirty_bisimulation: examples/blink_layout_dirty_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_layout_dirty_bisimulation.cpp -o blink_layout_dirty_bisimulation

blink_tokenizer_open_tag_bisimulation: examples/blink_tokenizer_open_tag_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenizer_open_tag_bisimulation.cpp -o blink_tokenizer_open_tag_bisimulation

blink_tokenizer_tag_name_bisimulation: examples/blink_tokenizer_tag_name_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenizer_tag_name_bisimulation.cpp -o blink_tokenizer_tag_name_bisimulation

blink_tokenizer_attribute_name_bisimulation: examples/blink_tokenizer_attribute_name_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenizer_attribute_name_bisimulation.cpp -o blink_tokenizer_attribute_name_bisimulation

blink_tokenizer_attribute_value_bisimulation: examples/blink_tokenizer_attribute_value_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenizer_attribute_value_bisimulation.cpp -o blink_tokenizer_attribute_value_bisimulation

blink_numeric_html_entity_bisimulation: examples/blink_numeric_html_entity_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_numeric_html_entity_bisimulation.cpp -o blink_numeric_html_entity_bisimulation

blink_html_named_entity: examples/blink_html_named_entity.cpp src/lambda_pi.hpp vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv
	$(CXX) $(CXXFLAGS) examples/blink_html_named_entity.cpp -o blink_html_named_entity

blink_html_named_entity_states: examples/blink_html_named_entity_states.cpp src/lambda_pi.hpp vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv
	$(CXX) $(CXXFLAGS) examples/blink_html_named_entity_states.cpp -o blink_html_named_entity_states

blink_html_named_entity_remainder: examples/blink_html_named_entity_remainder.cpp src/lambda_pi.hpp vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv
	$(CXX) $(CXXFLAGS) examples/blink_html_named_entity_remainder.cpp -o blink_html_named_entity_remainder

blink_segmented_entity_source_bisimulation: examples/blink_segmented_entity_source_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_segmented_entity_source_bisimulation.cpp -o blink_segmented_entity_source_bisimulation

blink_named_entity_segmented_composition: examples/blink_named_entity_segmented_composition.cpp src/lambda_pi.hpp vendor/chromium-src/third_party/blink/renderer/core/html/parser/html_entity_names.csv
	$(CXX) $(CXXFLAGS) examples/blink_named_entity_segmented_composition.cpp -o blink_named_entity_segmented_composition

blink_process_entity_bisimulation: examples/blink_process_entity_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_process_entity_bisimulation.cpp -o blink_process_entity_bisimulation

blink_tokenizer_attribute_entity_bisimulation: examples/blink_tokenizer_attribute_entity_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenizer_attribute_entity_bisimulation.cpp -o blink_tokenizer_attribute_entity_bisimulation

blink_dom_insert_html_element: examples/blink_dom_insert_html_element.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_dom_insert_html_element.cpp -o blink_dom_insert_html_element

blink_tree_builder_p_start_dom_insert: examples/blink_tree_builder_p_start_dom_insert.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tree_builder_p_start_dom_insert.cpp -o blink_tree_builder_p_start_dom_insert

blink_tokenized_p_start_dom: examples/blink_tokenized_p_start_dom.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_tokenized_p_start_dom.cpp -o blink_tokenized_p_start_dom

blink_pending_text_bisimulation: examples/blink_pending_text_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_pending_text_bisimulation.cpp -o blink_pending_text_bisimulation

blink_dom_text_flush_bisimulation: examples/blink_dom_text_flush_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_dom_text_flush_bisimulation.cpp -o blink_dom_text_flush_bisimulation

blink_paint_chunker_bisimulation: examples/blink_paint_chunker_bisimulation.cpp src/lambda_pi.hpp
	$(CXX) $(CXXFLAGS) examples/blink_paint_chunker_bisimulation.cpp -o blink_paint_chunker_bisimulation

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
	@echo "\n=== Running finite λΠ Blink tree-builder p-end bisimulation ==="
	./blink_tree_builder_p_end_bisimulation
	@echo "\n=== Running finite λΠ Blink tree-builder li-end bisimulation ==="
	./blink_tree_builder_li_end_bisimulation
	@echo "\n=== Running finite λΠ Blink tree-builder block-end bisimulation ==="
	./blink_tree_builder_block_end_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS cascade bisimulation ==="
	./blink_css_cascade_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS selector bisimulation ==="
	./blink_css_selector_bisimulation
	@echo "\n=== Running finite λΠ Blink CSS combinator bisimulation ==="
	./blink_css_combinator_bisimulation
	@echo "\n=== Running finite λΠ Blink selector collection and cascade winner ==="
	./blink_css_match_winner
	@echo "\n=== Running finite λΠ Blink selector-to-color style pipeline ==="
	./blink_css_selector_color
	@echo "\n=== Running finite λΠ Blink CSS revert-layer model ==="
	./blink_css_revert_layer
	@echo "\n=== Running finite λΠ CascadePriority layer normalization ==="
	./blink_css_priority_layer_normalization
	@echo "\n=== Running finite λΠ CascadePriority full ordering key ==="
	./blink_css_cascade_priority
	@echo "\n=== Running finite λΠ CascadeMap list and revert-layer model ==="
	./blink_css_cascade_map
	@echo "\n=== Running finite λΠ CascadeMap reversion plus color defaulting ==="
	./blink_css_revert_color
	@echo "\n=== Running finite λΠ CSS revert origin selection ==="
	./blink_css_revert_origin
	@echo "\n=== Running finite λΠ CSS revert origin plus color defaulting ==="
	./blink_css_revert_origin_color
	@echo "\n=== Running finite λΠ CSS revert origin with full priority ordering ==="
	./blink_css_revert_origin_full_priority
	@echo "\n=== Running finite λΠ Blink color defaulting model ==="
	./blink_css_color_defaulting
	@echo "\n=== Running finite λΠ Blink style recalc gate ==="
	./blink_style_recalc_gate
	@echo "\n=== Running finite λΠ Blink style recalc query dependencies ==="
	./blink_style_recalc_dependencies
	@echo "\n=== Running finite λΠ Blink style recalc child flag transform ==="
	./blink_style_recalc_children
	@echo "\n=== Running finite λΠ Blink style recalc traversal predicates ==="
	./blink_style_recalc_traversal
	./blink_style_recalc_copy
	@echo "\n=== Running finite λΠ Blink computed-style difference classifier ==="
	./blink_computed_style_difference
	@echo "\n=== Running finite λΠ Blink DOM insert element model ==="
	./blink_dom_insert_html_element
	./blink_tree_builder_p_start_dom_insert
	@echo "\n=== Running tokenized Blink p-start insertion projection ==="
	./blink_tokenized_p_start_dom
	@echo "\n=== Running Blink pending text accumulation and flush projection ==="
	./blink_pending_text_bisimulation
	@echo "\n=== Running Blink DOM Text-node flush/coalescing projection ==="
	./blink_dom_text_flush_bisimulation
	@echo "\n=== Running finite λΠ Blink layout dirty-bit model ==="
	./blink_layout_dirty_bisimulation
	@echo "\n=== Running finite λΠ Blink tokenizer open-tag dispatch model ==="
	./blink_tokenizer_open_tag_bisimulation
	@echo "\n=== Running finite λΠ Blink tokenizer TagName scan model ==="
	./blink_tokenizer_tag_name_bisimulation
	@echo "\n=== Running finite λΠ Blink tokenizer AttributeName scan model ==="
	./blink_tokenizer_attribute_name_bisimulation
	@echo "\n=== Running finite λΠ Blink tokenizer attribute-value model ==="
	./blink_tokenizer_attribute_value_bisimulation
	./blink_numeric_html_entity_bisimulation
	@echo "\n=== Running finite λΠ Blink named HTML entity lookup ==="
	./blink_html_named_entity
	@echo "\n=== Running finite λΠ Blink full-table named entity state model ==="
	./blink_html_named_entity_states
	@echo "\n=== Running finite λΠ Blink named entity remainder model ==="
	./blink_html_named_entity_remainder
	@echo "\n=== Running finite λΠ Blink segmented source and tokenizer suspension model ==="
	./blink_segmented_entity_source_bisimulation
	@echo "\n=== Running finite λΠ Blink full-table named-entity segmented tokenizer composition ==="
	./blink_named_entity_segmented_composition
	@echo "\n=== Running finite λΠ Blink ProcessEntity composition ==="
	./blink_process_entity_bisimulation
	@echo "\n=== Running finite λΠ Blink attribute character-reference composition ==="
	./blink_tokenizer_attribute_entity_bisimulation
	@echo "\n=== Running finite λΠ Blink PaintChunker grouping model ==="
	./blink_paint_chunker_bisimulation

clean:
	rm -f test_suite mltt_vectors automath_logic inverse_galois scheme_interpreter cpp_template_evaluator blink_rendering_model blink_parser_lifecycle blink_parser_bisimulation blink_tree_builder_p_start_bisimulation blink_tree_builder_p_end_bisimulation blink_tree_builder_li_end_bisimulation blink_tree_builder_block_end_bisimulation blink_css_cascade_bisimulation blink_css_selector_bisimulation blink_css_combinator_bisimulation blink_css_match_winner blink_css_selector_color blink_css_revert_layer blink_css_priority_layer_normalization blink_css_cascade_priority blink_css_cascade_map blink_css_revert_color blink_css_revert_origin blink_css_revert_origin_color blink_css_revert_origin_full_priority blink_css_color_defaulting blink_style_recalc_gate blink_style_recalc_dependencies blink_style_recalc_children blink_style_recalc_traversal blink_style_recalc_copy blink_computed_style_difference blink_dom_insert_html_element blink_tree_builder_p_start_dom_insert blink_tokenized_p_start_dom blink_layout_dirty_bisimulation blink_tokenizer_open_tag_bisimulation blink_tokenizer_tag_name_bisimulation blink_tokenizer_attribute_name_bisimulation blink_tokenizer_attribute_value_bisimulation blink_numeric_html_entity_bisimulation blink_html_named_entity blink_html_named_entity_states blink_html_named_entity_remainder blink_segmented_entity_source_bisimulation blink_named_entity_segmented_composition blink_process_entity_bisimulation blink_tokenizer_attribute_entity_bisimulation blink_paint_chunker_bisimulation blink_pending_text_bisimulation blink_dom_text_flush_bisimulation

.PHONY: all test clean

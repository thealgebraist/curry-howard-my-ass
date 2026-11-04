#include "../src/lambda_pi.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <vector>

// Exact finite submodel of the layer-order part of
// CascadePriority::ForLayerComparison. Origin and tree order are fixed to 0;
// only the low four bits of the 16-bit layer field vary. This does not model
// the complete packed 96-bit priority key.
using namespace lp;
static constexpr unsigned kLayerBits = 4;
static constexpr uint64_t kLayerMask4 = (1u << kLayerBits) - 1;
static uint64_t cpp_for_layer_projection(unsigned layer, bool important) {
  // Transcription of EncodeLayerOrder + the layer-mask normalization in
  // ForLayerComparison, embedded at Blink's actual field offsets.
  const uint64_t encoded_layer = layer ^ (important ? 0xffffu : 0u);
  const uint64_t low_bits = encoded_layer << 33;
  const uint32_t origin_encoded = important ? 0xFu : 0u;  // origin fixed to 0
  const uint32_t tree_encoded = important ? 0xffffu : 0u; // tree order fixed to 0
  const uint32_t high_bits = tree_encoded | (origin_encoded << 16);
  uint64_t bits = (low_bits >> 32) | (uint64_t{high_bits} << 32);
  if (bits & (uint64_t{1} << (19 + 32))) {
    bits ^= uint64_t{0xF} << 48;     // kOriginImportanceMask << 32
    bits ^= uint64_t{0xffff} << 32;  // kTreeOrderMask << 32
    bits ^= uint64_t{0xffff} << 1;   // kLayerOrderMask >> 32
  }
  bits >>= 1;  // kLayerOrderOffset - 32
  return bits & kLayerMask4;
}

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a),std::move(b)); }
static Rule rule(Ptr a,Ptr b) { return {std::move(a),std::move(b)}; }
static Ptr nat(unsigned n) { Ptr t=sym("z"); while(n--) t=app(sym("s"),t); return t; }
static unsigned nat_value(Ptr t) { unsigned n=0; while(auto a=std::get_if<App>(&t->v)) { auto h=std::get_if<Sym>(&a->fn->v); if(!h||h->name!="s") break; ++n; t=a->arg; } assert(std::get_if<Sym>(&t->v)&&std::get<Sym>(t->v).name=="z"); return n; }
static Ptr ap(Ptr f,std::initializer_list<Ptr> xs) { for(const auto& x:xs) f=app(f,x); return f; }

int main() {
  Signature sig; std::vector<Rule> rules; auto p=[](size_t i){return pvar(i);};
  auto n=sym("Nat"), b=sym("Bool");
  sig["Nat"]=type_sort(); sig["z"]=n; sig["s"]=fn(n,n);
  sig["Bool"]=type_sort(); sig["true"]=b; sig["false"]=b;
  sig["flip4"]=fn(n,n); sig["encodeLayer"]=fn(b,fn(n,n));
  sig["normalizeLayer"]=fn(b,fn(n,n)); sig["forLayer"]=fn(b,fn(n,n));
  rules.push_back(rule(ap(sym("encodeLayer"),{sym("false"),p(0)}),p(0)));
  rules.push_back(rule(ap(sym("encodeLayer"),{sym("true"),p(0)}),app(sym("flip4"),p(0))));
  rules.push_back(rule(ap(sym("normalizeLayer"),{sym("false"),p(0)}),p(0)));
  rules.push_back(rule(ap(sym("normalizeLayer"),{sym("true"),p(0)}),app(sym("flip4"),p(0))));
  rules.push_back(rule(ap(sym("forLayer"),{p(0),p(1)}),
    ap(sym("normalizeLayer"),{p(0),ap(sym("encodeLayer"),{p(0),p(1)})})));
  for(unsigned i=0;i<=kLayerMask4;++i) {
    Ptr lhs=app(sym("flip4"),nat(i));
    Ptr rhs=nat(kLayerMask4 ^ i);
    rules.push_back(rule(std::move(lhs),std::move(rhs)));
  }
  auto bool_term=[](bool x){return sym(x?"true":"false");};
  unsigned comparisons=0;
  for(bool important : {false,true}) for(unsigned layer=0;layer<=kLayerMask4;++layer) {
    Ptr encoded=ap(sym("encodeLayer"),{bool_term(important),nat(layer)});
    auto encoded_type=infer({},encoded,sig,rules);
    assert(encoded_type&&conv(*encoded_type,n,rules));
    const unsigned encoded_value=nat_value(nf(encoded,rules));
    const unsigned expected_encoded=layer ^ (important?static_cast<unsigned>(kLayerMask4):0u);
    // Since the fixture varies only four low bits, this is the exact projection
    // of Blink's 16-bit XOR mask to those bits.
    assert(encoded_value==expected_encoded);

    Ptr term=ap(sym("forLayer"),{bool_term(important),nat(layer)});
    auto ty=infer({},term,sig,rules); assert(ty&&conv(*ty,n,rules));
    const unsigned model_value=nat_value(nf(term,rules));
    const uint64_t cpp_value=cpp_for_layer_projection(layer,important);
    assert(model_value==cpp_value);
    ++comparisons;
  }
  std::cout << "CascadePriority layer normalization agrees on " << comparisons << " finite field cases\n";
  std::cout << "Scope: 4-bit layer projection, both importance states; origin/tree fixed; packed key otherwise omitted.\n";
}

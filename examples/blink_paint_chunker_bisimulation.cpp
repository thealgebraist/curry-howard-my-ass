#include "../src/lambda_pi.hpp"
#include <cassert>
#include <optional>
#include <vector>

using namespace lp;

enum class Property { A, B, C };
struct Item { Property property; bool special; }; // special = foreign layer or scrollbar
using Items = std::vector<Item>;
struct ChunkState {
    std::optional<Property> last_properties;
    bool force_new_chunk = true;
    unsigned chunk_count = 0;
    unsigned current_chunk = 0;
    std::vector<unsigned> assigned_chunk_ids;
};

static ChunkState cpp_run(const Items& items) {
    ChunkState state;
    for (const Item item : items) {
        const bool force_before = state.force_new_chunk || item.special;
        const bool create = force_before || !state.last_properties ||
                            *state.last_properties != item.property;
        if (create) {
            state.current_chunk = state.chunk_count++;
            state.last_properties = item.property;
            state.force_new_chunk = false;
        }
        state.assigned_chunk_ids.push_back(state.current_chunk);
        if (item.special) state.force_new_chunk = true;
    }
    return state;
}

static Ptr fn(Ptr a,Ptr b){return arrow(std::move(a),std::move(b));}
static Rule rule(Ptr a,Ptr b){return{std::move(a),std::move(b)};}
static Ptr nat(unsigned n){Ptr t=sym("zero");while(n--)t=app(sym("succ"),t);return t;}
static Ptr prop_term(Property p){return sym(p==Property::A?"propA":p==Property::B?"propB":"propC");}
static Ptr items_term(const Items& items){
    Ptr result=sym("nilItems");
    for(auto i=items.rbegin();i!=items.rend();++i)
        result=apps(sym("consItem"),apps(sym("displayItem"),prop_term(i->property),sym(i->special?"true":"false")),result);
    return result;
}
static Ptr ids_term(const std::vector<unsigned>& ids){
    Ptr result=sym("nilIds");
    for(auto i=ids.rbegin();i!=ids.rend();++i)
        result=apps(sym("consId"),nat(*i),result);
    return result;
}
static Ptr state_term(const ChunkState& s){
    Ptr last=s.last_properties?app(sym("someProperty"),prop_term(*s.last_properties)):sym("noProperty");
    return apps(sym("chunkState"),last,sym(s.force_new_chunk?"true":"false"),nat(s.chunk_count),
                nat(s.current_chunk),ids_term(s.assigned_chunk_ids));
}

int main(){
    Signature sig;std::vector<Rule> rules;auto p=[](size_t i){return pvar(i);};
    const Ptr boolean=sym("Bool"),property=sym("Property"),option=sym("OptionProperty");
    const Ptr item=sym("DisplayItem"),items=sym("DisplayItems"),ids=sym("ChunkIds");
    const Ptr state=sym("ChunkerState");
    sig["Bool"]=type_sort();sig["true"]=boolean;sig["false"]=boolean;
    sig["Property"]=type_sort();sig["propA"]=property;sig["propB"]=property;sig["propC"]=property;
    sig["OptionProperty"]=type_sort();sig["noProperty"]=option;sig["someProperty"]=fn(property,option);
    sig["DisplayItem"]=type_sort();sig["displayItem"]=fn(property,fn(boolean,item));
    sig["DisplayItems"]=type_sort();sig["nilItems"]=items;sig["consItem"]=fn(item,fn(items,items));
    sig["ChunkIds"]=type_sort();sig["nilIds"]=ids;sig["consId"]=fn(sym("Nat"),fn(ids,ids));
    sig["Nat"]=type_sort();sig["zero"]=sym("Nat");sig["succ"]=fn(sym("Nat"),sym("Nat"));
    sig["ChunkerState"]=type_sort();
    sig["chunkState"]=fn(option,fn(boolean,fn(sym("Nat"),fn(sym("Nat"),fn(ids,state)))));
    sig["newChunkState"]=fn(state,fn(item,state));sig["reuseChunkState"]=fn(state,fn(item,state));
    sig["stepByForce"]=fn(boolean,fn(state,fn(item,state)));sig["stepChunker"]=fn(state,fn(item,state));
    sig["eqProperty"]=fn(property,fn(property,boolean));sig["ifState"]=fn(boolean,fn(state,fn(state,state)));
    sig["snocId"]=fn(ids,fn(sym("Nat"),ids));sig["runItems"]=fn(state,fn(items,state));
    sig["initialState"]=state;sig["inputAt"]=fn(sym("Case"),items);sig["nativeAt"]=fn(sym("Case"),state);
    sig["semanticAt"]=fn(sym("Case"),state);sig["Case"]=type_sort();
    sig["andB"]=fn(boolean,fn(boolean,boolean));sig["notB"]=fn(boolean,boolean);
    sig["eqBool"]=fn(boolean,fn(boolean,boolean));
    sig["eqNat"]=fn(sym("Nat"),fn(sym("Nat"),boolean));sig["eqIds"]=fn(ids,fn(ids,boolean));
    sig["eqOption"]=fn(option,fn(option,boolean));sig["eqState"]=fn(state,fn(state,boolean));
    sig["checkAt"]=fn(sym("Case"),boolean);

    for(Property a:{Property::A,Property::B,Property::C})for(Property b:{Property::A,Property::B,Property::C})
        rules.push_back(rule(apps(sym("eqProperty"),prop_term(a),prop_term(b)),sym(a==b?"true":"false")));
    rules.push_back(rule(apps(sym("snocId"),sym("nilIds"),p(0)),apps(sym("consId"),p(0),sym("nilIds"))));
    rules.push_back(rule(apps(sym("snocId"),apps(sym("consId"),p(0),p(1)),p(2)),
                         apps(sym("consId"),p(0),apps(sym("snocId"),p(1),p(2)))));
    rules.push_back(rule(apps(sym("newChunkState"),
        apps(sym("chunkState"),p(0),p(1),p(2),p(3),p(4)),
        apps(sym("displayItem"),p(5),p(6))),
        apps(sym("chunkState"),app(sym("someProperty"),p(5)),p(6),app(sym("succ"),p(2)),
             p(2),apps(sym("snocId"),p(4),p(2)))));
    rules.push_back(rule(apps(sym("reuseChunkState"),
        apps(sym("chunkState"),p(0),sym("false"),p(1),p(2),p(3)),
        apps(sym("displayItem"),p(4),sym("false"))),
        apps(sym("chunkState"),p(0),sym("false"),p(1),p(2),apps(sym("snocId"),p(3),p(2)))));
    rules.push_back(rule(apps(sym("stepByForce"),sym("true"),p(0),p(1)),apps(sym("newChunkState"),p(0),p(1))));
    rules.push_back(rule(apps(sym("stepByForce"),sym("false"),p(0),apps(sym("displayItem"),p(1),sym("true"))),
                         apps(sym("newChunkState"),p(0),apps(sym("displayItem"),p(1),sym("true")))));
    rules.push_back(rule(apps(sym("stepByForce"),sym("false"),
        apps(sym("chunkState"),sym("noProperty"),sym("false"),p(0),p(1),p(2)),p(3)),
        apps(sym("newChunkState"),apps(sym("chunkState"),sym("noProperty"),sym("false"),p(0),p(1),p(2)),p(3))));
    rules.push_back(rule(apps(sym("stepByForce"),sym("false"),
        apps(sym("chunkState"),app(sym("someProperty"),p(0)),sym("false"),p(1),p(2),p(3)),
        apps(sym("displayItem"),p(4),sym("false"))),
        apps(sym("ifState"),apps(sym("eqProperty"),p(0),p(4)),
            apps(sym("reuseChunkState"),apps(sym("chunkState"),app(sym("someProperty"),p(0)),sym("false"),p(1),p(2),p(3)),
                 apps(sym("displayItem"),p(4),sym("false"))),
            apps(sym("newChunkState"),apps(sym("chunkState"),app(sym("someProperty"),p(0)),sym("false"),p(1),p(2),p(3)),
                 apps(sym("displayItem"),p(4),sym("false"))))));
    rules.push_back(rule(apps(sym("stepChunker"),p(0),p(1)),apps(sym("stepByForce"),
        apps(sym("orB"),apps(sym("stateForced"),p(0)),apps(sym("itemSpecial"),p(1))),p(0),p(1))));
    sig["stateForced"]=fn(state,boolean);sig["itemSpecial"]=fn(item,boolean);sig["orB"]=fn(boolean,fn(boolean,boolean));
    rules.push_back(rule(app(sym("stateForced"),apps(sym("chunkState"),p(0),p(1),p(2),p(3),p(4))),p(1)));
    rules.push_back(rule(app(sym("itemSpecial"),apps(sym("displayItem"),p(0),p(1))),p(1)));
    rules.push_back(rule(apps(sym("orB"),sym("true"),p(0)),sym("true")));
    rules.push_back(rule(apps(sym("orB"),sym("false"),p(0)),p(0)));
    rules.push_back(rule(apps(sym("ifState"),sym("true"),p(0),p(1)),p(0)));
    rules.push_back(rule(apps(sym("ifState"),sym("false"),p(0),p(1)),p(1)));
    rules.push_back(rule(apps(sym("runItems"),p(0),sym("nilItems")),p(0)));
    rules.push_back(rule(apps(sym("runItems"),p(0),apps(sym("consItem"),p(1),p(2))),
                         apps(sym("runItems"),apps(sym("stepChunker"),p(0),p(1)),p(2))));
    rules.push_back(rule(apps(sym("eqNat"),sym("zero"),sym("zero")),sym("true")));
    rules.push_back(rule(apps(sym("eqNat"),sym("zero"),app(sym("succ"),p(0))),sym("false")));
    rules.push_back(rule(apps(sym("eqNat"),app(sym("succ"),p(0)),sym("zero")),sym("false")));
    rules.push_back(rule(apps(sym("eqNat"),app(sym("succ"),p(0)),app(sym("succ"),p(1))),apps(sym("eqNat"),p(0),p(1))));
    rules.push_back(rule(apps(sym("andB"),sym("true"),sym("true")),sym("true")));
    rules.push_back(rule(apps(sym("andB"),sym("true"),sym("false")),sym("false")));
    rules.push_back(rule(apps(sym("andB"),sym("false"),p(0)),sym("false")));
    rules.push_back(rule(apps(sym("eqBool"),sym("true"),sym("true")),sym("true")));
    rules.push_back(rule(apps(sym("eqBool"),sym("true"),sym("false")),sym("false")));
    rules.push_back(rule(apps(sym("eqBool"),sym("false"),sym("true")),sym("false")));
    rules.push_back(rule(apps(sym("eqBool"),sym("false"),sym("false")),sym("true")));
    rules.push_back(rule(apps(sym("eqIds"),sym("nilIds"),sym("nilIds")),sym("true")));
    rules.push_back(rule(apps(sym("eqIds"),sym("nilIds"),apps(sym("consId"),p(0),p(1))),sym("false")));
    rules.push_back(rule(apps(sym("eqIds"),apps(sym("consId"),p(0),p(1)),sym("nilIds")),sym("false")));
    rules.push_back(rule(apps(sym("eqIds"),apps(sym("consId"),p(0),p(1)),apps(sym("consId"),p(2),p(3))),
        apps(sym("andB"),apps(sym("eqNat"),p(0),p(2)),apps(sym("eqIds"),p(1),p(3)))));
    rules.push_back(rule(apps(sym("eqOption"),sym("noProperty"),sym("noProperty")),sym("true")));
    rules.push_back(rule(apps(sym("eqOption"),sym("noProperty"),app(sym("someProperty"),p(0))),sym("false")));
    rules.push_back(rule(apps(sym("eqOption"),app(sym("someProperty"),p(0)),sym("noProperty")),sym("false")));
    rules.push_back(rule(apps(sym("eqOption"),app(sym("someProperty"),p(0)),app(sym("someProperty"),p(1))),apps(sym("eqProperty"),p(0),p(1))));
    rules.push_back(rule(apps(sym("eqState"),
        apps(sym("chunkState"),p(0),p(1),p(2),p(3),p(4)),
        apps(sym("chunkState"),p(5),p(6),p(7),p(8),p(9))),
        apps(sym("andB"),apps(sym("eqOption"),p(0),p(5)),
          apps(sym("andB"),apps(sym("eqBool"),p(1),p(6)),
            apps(sym("andB"),apps(sym("eqNat"),p(2),p(7)),
              apps(sym("andB"),apps(sym("eqNat"),p(3),p(8)),apps(sym("eqIds"),p(4),p(9))))))));
    rules.push_back(rule(app(sym("semanticAt"),p(0)),apps(sym("runItems"),sym("initialState"),app(sym("inputAt"),p(0)))));
    rules.push_back(rule(apps(sym("checkAt"),p(0)),apps(sym("eqState"),app(sym("nativeAt"),p(0)),app(sym("semanticAt"),p(0)))));

    const std::vector<Items> cases={
        {},
        {{Property::A,false},{Property::A,false},{Property::A,false}},
        {{Property::A,false},{Property::B,false},{Property::B,false}},
        {{Property::A,false},{Property::A,true},{Property::A,false}},
        {{Property::B,true},{Property::B,false}},
        {{Property::A,false},{Property::B,false},{Property::B,true},{Property::B,false},{Property::A,false}},
        {{Property::A,false},{Property::B,false},{Property::A,false}},
        {{Property::C,true},{Property::C,true},{Property::C,false}}};
    sig["initialState"]=state;
    const Ptr empty_state=apps(sym("chunkState"),sym("noProperty"),sym("true"),nat(0),nat(0),sym("nilIds"));
    rules.push_back(rule(sym("initialState"),empty_state));
    for(size_t i=0;i<cases.size();++i){
        const Ptr id=sym(("paintCase"+std::to_string(i)).c_str());sig[to_string(id)]=sym("Case");
        const ChunkState native=cpp_run(cases[i]);
        rules.push_back(rule(app(sym("inputAt"),id),items_term(cases[i])));
        rules.push_back(rule(app(sym("nativeAt"),id),state_term(native)));
        const Ptr check=app(sym("checkAt"),id);const auto type=infer({},check,sig,rules);
        assert(type&&conv(*type,boolean,rules));assert(conv(nf(check,rules),sym("true"),rules));
    }
    std::println("Blink PaintChunker grouping: {} finite item sequences agree between C++ and typed λΠ",cases.size());
    std::println("Scope: property-state grouping and forced boundaries around foreign-layer/scrollbar items.");
}

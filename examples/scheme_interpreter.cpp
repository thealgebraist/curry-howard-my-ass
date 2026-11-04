#include "../src/lambda_pi.hpp"
#include <cassert>

// An object-language evaluator implemented with λΠ constants and rewrite
// equations. The object language is a small dynamically typed Scheme core.
using namespace lp;

static Ptr c(std::string f, std::initializer_list<Ptr> xs = {}) {
    Ptr t = sym(std::move(f));
    for (const auto& x : xs) t = app(t, x);
    return t;
}
static Rule rw(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }
static Ptr nat(size_t n) { Ptr x = sym("zero"); while (n--) x = app(sym("succ"), x); return x; }
static Ptr dlist(const std::vector<Ptr>& xs) {
    Ptr out = sym("vnil");
    for (auto i = xs.rbegin(); i != xs.rend(); ++i) out = c("vpair", {*i, out});
    return out;
}
static Ptr dsym(const std::string& name) { return c("vsym", {sym(name)}); }
static Ptr reify_names(Ptr xs) {
    if (auto s = std::get_if<Sym>(&xs->v); s && s->name == "nilN") return sym("vnil");
    auto [head, args] = unroll(xs);
    auto s = std::get_if<Sym>(&head->v);
    if (!s || s->name != "consN" || args.size() != 2) return sym("vnil");
    auto name = std::get_if<Sym>(&args[0]->v);
    return c("vpair", {dsym(name ? name->name : "?"), reify_names(args[1])});
}
static Ptr reify_expr_list(Ptr xs) {
    if (auto s = std::get_if<Sym>(&xs->v); s && s->name == "nilE") return sym("vnil");
    auto [head, args] = unroll(xs);
    auto s = std::get_if<Sym>(&head->v);
    if (!s || s->name != "consE" || args.size() != 2) return sym("vnil");
    extern Ptr reify_scheme_expr(Ptr);
    return c("vpair", {reify_scheme_expr(args[0]), reify_expr_list(args[1])});
}
Ptr reify_scheme_expr(Ptr term) {
    auto [head, args] = unroll(term);
    auto s = std::get_if<Sym>(&head->v);
    if (!s) return sym("vnil");
    if (s->name == "quote" && args.size() == 1) return dlist({dsym("tag-quote"), args[0]});
    if (s->name == "var" && args.size() == 1) {
        auto nm = std::get_if<Sym>(&args[0]->v);
        return dlist({dsym("tag-var"), dsym(nm ? nm->name : "?")});
    }
    if (s->name == "lambda" && args.size() == 2)
        return dlist({dsym("tag-lambda"), reify_names(args[0]), reify_scheme_expr(args[1])});
    if (s->name == "app" && args.size() == 2)
        return dlist({dsym("tag-app"), reify_scheme_expr(args[0]), reify_expr_list(args[1])});
    if (s->name == "if" && args.size() == 3)
        return dlist({dsym("tag-if"), reify_scheme_expr(args[0]), reify_scheme_expr(args[1]), reify_scheme_expr(args[2])});
    if (s->name == "lit" && args.size() == 1) return dlist({dsym("tag-lit"), c("vnat", {args[0]})});
    return sym("vnil");
}
static Ptr sv(const std::string& name) { return c("var", {sym(name)}); }
static Ptr sq(Ptr datum) { return c("quote", {std::move(datum)}); }
static Ptr slambda(const std::string& name, Ptr body) { return c("lambda", {c("consN", {sym(name), sym("nilN")}), std::move(body)}); }
static Ptr sapp(Ptr fn, Ptr arg) { return c("app", {std::move(fn), c("consE", {std::move(arg), sym("nilE")})}); }
static Ptr sif(Ptr test, Ptr yes, Ptr no) { return c("if", {std::move(test), std::move(yes), std::move(no)}); }
static Ptr scall1(const std::string& name, Ptr arg) { return sapp(sv(name), std::move(arg)); }
static Ptr scall2(const std::string& name, Ptr a, Ptr b) { return sapp(scall1(name, std::move(a)), std::move(b)); }

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto pv = [](size_t n) { return pvar(n); };
    auto n = sym("Nat");
    auto e = sym("Expr");
    auto value = sym("Value");
    auto result = sym("Result");
    auto env = sym("Env");
    auto names = sym("Names");
    auto exprs = sym("Exprs");
    auto values = sym("Values");
    auto primitive = sym("Primitive");
    auto arr = [](Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); };

    // Built-in natural numbers are the numeric value domain.
    sig["Nat"] = type_sort(); sig["zero"] = n; sig["succ"] = arr(n, n);
    sig["add"] = arr(n, arr(n, n));
    rules.push_back(rw(apps(sym("add"), sym("zero"), pv(0)), pv(0)));
    rules.push_back(rw(apps(sym("add"), app(sym("succ"), pv(0)), pv(1)),
                       app(sym("succ"), apps(sym("add"), pv(0), pv(1)))));

    sig["Name"] = type_sort(); sig["Expr"] = type_sort(); sig["Value"] = type_sort();
    sig["Primitive"] = type_sort();
    sig["Env"] = type_sort(); sig["Result"] = type_sort();
    sig["Names"] = type_sort(); sig["Exprs"] = type_sort(); sig["Values"] = type_sort();
    for (const auto* name : {"x", "self", "applyObj", "expr", "env", "proc", "arg", "f", "z", "v",
                             "car", "cdr", "cons", "eq?", "assq", "apply", "dataClosure?", "tag-lit", "tag-quote",
                             "tag-var", "tag-lambda", "tag-app", "tag-if", "tag-closure"})
        sig[name] = sym("Name");
    sig["nilN"] = names; sig["consN"] = arr(sym("Name"), arr(names, names));
    sig["nilE"] = exprs; sig["consE"] = arr(e, arr(exprs, exprs));
    sig["nilV"] = values; sig["consV"] = arr(value, arr(values, values));
    sig["empty"] = env;
    sig["bind"] = arr(sym("Name"), arr(value, arr(env, env)));
    sig["vnat"] = arr(n, value); sig["vtrue"] = value; sig["vfalse"] = value;
    sig["venv"] = arr(env, value);
    sig["vnil"] = value; sig["vsym"] = arr(sym("Name"), value);
    sig["vpair"] = arr(value, arr(value, value)); sig["vprim"] = arr(primitive, value);
    sig["vpartial"] = arr(primitive, arr(value, value));
    for (auto p : {"pcons", "pcar", "pcdr", "peq", "passq", "papply", "pdataClosure"}) sig[p] = primitive;
    sig["closure"] = arr(names, arr(e, arr(env, value)));
    sig["ok"] = arr(value, result); sig["error"] = arr(sym("Error"), result);
    sig["Error"] = type_sort(); sig["unbound"] = sym("Error"); sig["arity"] = sym("Error");
    sig["notProc"] = sym("Error"); sig["badExpr"] = sym("Error");

    sig["lit"] = arr(n, e); sig["quote"] = arr(value, e); sig["var"] = arr(sym("Name"), e);
    sig["lambda"] = arr(names, arr(e, e)); sig["app"] = arr(e, arr(exprs, e));
    sig["if"] = arr(e, arr(e, arr(e, e)));
    sig["plus"] = arr(e, arr(e, e));

    sig["eval"] = arr(e, arr(env, result));
    sig["lookup"] = arr(sym("Name"), arr(env, result));
    sig["evalList"] = arr(exprs, arr(env, arr(values, result)));
    sig["applyValue"] = arr(value, arr(values, result));
    sig["bindAll"] = arr(names, arr(values, arr(env, result)));
    sig["bindTail"] = arr(result, arr(sym("Name"), arr(value, result)));
    sig["applyBound"] = arr(result, arr(e, result));
    sig["ifResult"] = arr(result, arr(e, arr(e, arr(env, result))));
    sig["fnResult"] = arr(result, arr(exprs, arr(env, result)));
    sig["argsResult"] = arr(value, arr(result, result));
    sig["plusResult"] = arr(result, arr(result, result));
    sig["evalListHead"] = arr(result, arr(exprs, arr(env, arr(values, result))));
    sig["applyArgs"] = arr(value, arr(result, result));
    sig["plusLeft"] = arr(result, arr(e, arr(env, result)));
    sig["appendV"] = arr(values, arr(value, values));
    sig["assqData"] = arr(value, arr(value, value));
    sig["valuesFromData"] = arr(value, values);

    // Environment operations and parallel formal/actual binding.
    rules.push_back(rw(apps(sym("lookup"), pv(0), c("bind", {pv(0), pv(1), pv(2)})), c("ok", {pv(1)})));
    rules.push_back(rw(apps(sym("lookup"), pv(0), c("bind", {pv(1), pv(2), pv(3)})), apps(sym("lookup"), pv(0), pv(3))));
    rules.push_back(rw(apps(sym("lookup"), pv(0), sym("empty")), c("error", {sym("unbound")})));
    rules.push_back(rw(apps(sym("appendV"), sym("nilV"), pv(0)), c("consV", {pv(0), sym("nilV")})));
    rules.push_back(rw(apps(sym("appendV"), c("consV", {pv(0), pv(1)}), pv(2)),
                       c("consV", {pv(0), apps(sym("appendV"), pv(1), pv(2))})));
    // Scheme's assq over association lists; false denotes a missing key.
    rules.push_back(rw(apps(sym("assqData"), c("vsym", {pv(0)}), c("vpair", {c("vpair", {c("vsym", {pv(0)}), pv(1)}), pv(2)})),
                       c("vpair", {c("vsym", {pv(0)}), pv(1)})));
    rules.push_back(rw(apps(sym("assqData"), c("vsym", {pv(0)}), c("vpair", {c("vpair", {c("vsym", {pv(1)}), pv(2)}), pv(3)})),
                       apps(sym("assqData"), c("vsym", {pv(0)}), pv(3))));
    rules.push_back(rw(apps(sym("assqData"), pv(0), sym("vnil")), sym("vfalse")));
    rules.push_back(rw(apps(sym("valuesFromData"), sym("vnil")), sym("nilV")));
    rules.push_back(rw(apps(sym("valuesFromData"), c("vpair", {pv(0), pv(1)})),
                       c("consV", {pv(0), apps(sym("valuesFromData"), pv(1))})));
    rules.push_back(rw(apps(sym("bindAll"), sym("nilN"), sym("nilV"), pv(0)), c("ok", {c("venv", {pv(0)})})));
    rules.push_back(rw(apps(sym("bindAll"), c("consN", {pv(0), pv(1)}), c("consV", {pv(2), pv(3)}), pv(4)),
                       apps(sym("bindTail"), apps(sym("bindAll"), pv(1), pv(3), pv(4)), pv(0), pv(2))));
    rules.push_back(rw(apps(sym("bindTail"), c("ok", {c("venv", {pv(0)})}), pv(1), pv(2)),
                       c("ok", {c("venv", {c("bind", {pv(1), pv(2), pv(0)})})})));
    rules.push_back(rw(apps(sym("bindTail"), c("error", {pv(0)}), pv(1), pv(2)), c("error", {pv(0)})));
    rules.push_back(rw(apps(sym("bindAll"), pv(0), pv(1), pv(2)), c("error", {sym("arity")})));

    // Evaluate operand lists left-to-right.
    rules.push_back(rw(apps(sym("evalList"), sym("nilE"), pv(0), pv(1)), c("ok", {pv(1)})));
    rules.push_back(rw(apps(sym("evalList"), c("consE", {pv(0), pv(1)}), pv(2), pv(3)),
                       apps(sym("evalListHead"), apps(sym("eval"), pv(0), pv(2)), pv(1), pv(2), pv(3))));
    rules.push_back(rw(apps(sym("evalListHead"), c("ok", {pv(0)}), pv(1), pv(2), pv(3)),
                       apps(sym("evalList"), pv(1), pv(2), apps(sym("appendV"), pv(3), pv(0)))));
    rules.push_back(rw(apps(sym("evalListHead"), c("error", {pv(0)}), pv(1), pv(2), pv(3)), c("error", {pv(0)})));

    // Evaluation rules: literals, quote, variables, lambdas, conditionals,
    // application, and numeric addition.
    rules.push_back(rw(apps(sym("eval"), c("lit", {pv(0)}), pv(1)), c("ok", {c("vnat", {pv(0)})})));
    rules.push_back(rw(apps(sym("eval"), c("quote", {pv(0)}), pv(1)), c("ok", {pv(0)})));
    rules.push_back(rw(apps(sym("eval"), c("var", {pv(0)}), pv(1)), apps(sym("lookup"), pv(0), pv(1))));
    rules.push_back(rw(apps(sym("eval"), c("lambda", {pv(0), pv(1)}), pv(2)), c("ok", {c("closure", {pv(0), pv(1), pv(2)})})));
    rules.push_back(rw(apps(sym("eval"), c("if", {pv(0), pv(1), pv(2)}), pv(3)),
                       apps(sym("ifResult"), apps(sym("eval"), pv(0), pv(3)), pv(1), pv(2), pv(3))));
    rules.push_back(rw(apps(sym("ifResult"), c("ok", {sym("vfalse")}), pv(0), pv(1), pv(2)), apps(sym("eval"), pv(1), pv(2))));
    rules.push_back(rw(apps(sym("ifResult"), c("ok", {pv(0)}), pv(1), pv(2), pv(3)), apps(sym("eval"), pv(1), pv(3))));
    rules.push_back(rw(apps(sym("ifResult"), c("error", {pv(0)}), pv(1), pv(2), pv(3)), c("error", {pv(0)})));
    rules.push_back(rw(apps(sym("eval"), c("app", {pv(0), pv(1)}), pv(2)),
                       apps(sym("fnResult"), apps(sym("eval"), pv(0), pv(2)), pv(1), pv(2))));
    rules.push_back(rw(apps(sym("fnResult"), c("ok", {pv(0)}), pv(1), pv(2)),
                       apps(sym("applyArgs"), pv(0), apps(sym("evalList"), pv(1), pv(2), sym("nilV")))));
    rules.push_back(rw(apps(sym("applyArgs"), pv(0), c("ok", {pv(1)})), apps(sym("applyValue"), pv(0), pv(1))));
    rules.push_back(rw(apps(sym("applyArgs"), pv(0), c("error", {pv(1)})), c("error", {pv(1)})));
    rules.push_back(rw(apps(sym("fnResult"), c("error", {pv(0)}), pv(1), pv(2)), c("error", {pv(0)})));
    rules.push_back(rw(apps(sym("applyValue"), c("closure", {pv(0), pv(1), pv(2)}), pv(3)),
                       apps(sym("applyBound"), apps(sym("bindAll"), pv(0), pv(3), pv(2)), pv(1))));
    rules.push_back(rw(apps(sym("applyBound"), c("ok", {c("venv", {pv(0)})}), pv(1)), apps(sym("eval"), pv(1), pv(0))));
    rules.push_back(rw(apps(sym("applyBound"), c("error", {pv(0)}), pv(1)), c("error", {pv(0)})));
    // Primitive procedures available to the Scheme program (data operations,
    // association lookup, and application of primitive/host closures).
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("pcons")}), c("consV", {pv(0), sym("nilV")})),
                       c("ok", {c("vpartial", {sym("pcons"), pv(0)})})));
    rules.push_back(rw(apps(sym("applyValue"), c("vpartial", {sym("pcons"), pv(0)}), c("consV", {pv(1), sym("nilV")})),
                       c("ok", {c("vpair", {pv(0), pv(1)})})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("pcar")}), c("consV", {c("vpair", {pv(0), pv(1)}), sym("nilV")})), c("ok", {pv(0)})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("pcdr")}), c("consV", {c("vpair", {pv(0), pv(1)}), sym("nilV")})), c("ok", {pv(1)})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("peq")}), c("consV", {pv(0), sym("nilV")})),
                       c("ok", {c("vpartial", {sym("peq"), pv(0)})})));
    rules.push_back(rw(apps(sym("applyValue"), c("vpartial", {sym("peq"), c("vsym", {pv(0)})}), c("consV", {c("vsym", {pv(0)}), sym("nilV")})), c("ok", {sym("vtrue")})));
    rules.push_back(rw(apps(sym("applyValue"), c("vpartial", {sym("peq"), pv(0)}), c("consV", {pv(1), sym("nilV")})), c("ok", {sym("vfalse")})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("passq")}), c("consV", {pv(0), sym("nilV")})),
                       c("ok", {c("vpartial", {sym("passq"), pv(0)})})));
    rules.push_back(rw(apps(sym("applyValue"), c("vpartial", {sym("passq"), pv(0)}), c("consV", {pv(1), sym("nilV")})), c("ok", {apps(sym("assqData"), pv(0), pv(1))})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("papply")}), c("consV", {pv(0), sym("nilV")})),
                       c("ok", {c("vpartial", {sym("papply"), pv(0)})})));
    rules.push_back(rw(apps(sym("applyValue"), c("vpartial", {sym("papply"), pv(0)}), c("consV", {pv(1), sym("nilV")})),
                       apps(sym("applyValue"), pv(0), apps(sym("valuesFromData"), pv(1)))));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("pdataClosure")}),
                            c("consV", {c("vpair", {c("vsym", {sym("tag-closure")}), pv(0)}), sym("nilV")})),
                       c("ok", {sym("vtrue")})));
    rules.push_back(rw(apps(sym("applyValue"), c("vprim", {sym("pdataClosure")}), c("consV", {pv(0), sym("nilV")})),
                       c("ok", {sym("vfalse")})));
    rules.push_back(rw(apps(sym("applyValue"), pv(0), pv(1)), c("error", {sym("notProc")})));
    rules.push_back(rw(apps(sym("eval"), c("plus", {pv(0), pv(1)}), pv(2)),
                       apps(sym("plusLeft"), apps(sym("eval"), pv(0), pv(2)), pv(1), pv(2))));
    rules.push_back(rw(apps(sym("plusLeft"), c("ok", {pv(0)}), pv(1), pv(2)),
                       apps(sym("plusResult"), c("ok", {pv(0)}), apps(sym("eval"), pv(1), pv(2)))));
    rules.push_back(rw(apps(sym("plusResult"), c("ok", {c("vnat", {pv(0)})}), c("ok", {c("vnat", {pv(1)})})),
                       c("ok", {c("vnat", {apps(sym("add"), pv(0), pv(1))})})));
    rules.push_back(rw(apps(sym("plusResult"), c("error", {pv(0)}), pv(1)), c("error", {pv(0)})));
    rules.push_back(rw(apps(sym("plusResult"), pv(0), c("error", {pv(1)})), c("error", {pv(1)})));
    rules.push_back(rw(apps(sym("eval"), pv(0), pv(1)), c("error", {sym("badExpr")})));

    // Typed identity application: ((lambda (x) x) (quote 7)).
    Ptr x = sym("x");
    Ptr expr = c("app", {
        c("lambda", {c("consN", {x, sym("nilN")}), c("var", {x})}),
        c("consE", {c("quote", {c("vnat", {nat(7)})}), sym("nilE")})
    });
    Ptr run = apps(sym("eval"), expr, sym("empty"));
    auto inferred = infer({}, run, sig, rules);
    Ptr expected = c("ok", {c("vnat", {nat(7)})});
    Ptr reduced = nf(run, rules);
    if (!inferred) std::println("Type error: {}", inferred.error());
    assert(inferred && conv(*inferred, result, rules));
    assert(conv(reduced, expected, rules));
    std::println("Typed identity application: {}", to_string(reduced));
    std::println("Term type checks as Result: {}", static_cast<bool>(inferred));
    std::println("Evaluator equations: {} rewrite rules", rules.size());

    // Scheme source for a metacircular interpreter over tagged S-expressions.
    // It is written using only lambdas, application, if, quote, and the small
    // Scheme primitive basis (cons/car/cdr/eq?/assq/apply).
    auto v = [](const char* name) { return sv(name); };
    auto quoted_symbol = [](const char* name) { return sq(dsym(name)); };
    auto car = [&](Ptr t) { return scall1("car", std::move(t)); };
    auto cdr = [&](Ptr t) { return scall1("cdr", std::move(t)); };
    auto field = [&](Ptr t, size_t i) {
        for (size_t k = 0; k < i; ++k) t = cdr(std::move(t));
        return car(std::move(t));
    };
    auto self_call = [&](Ptr code, Ptr target_env) {
        return sapp(sapp(v("self"), std::move(code)), std::move(target_env));
    };
    auto make_pair = [&](Ptr a, Ptr b) { return scall2("cons", std::move(a), std::move(b)); };
    auto same_tag = [&](Ptr t, const char* tag) {
        return scall2("eq?", car(std::move(t)), quoted_symbol(tag));
    };

    // applyObj interprets closure data itself; only primitive procedures are
    // delegated to the λΠ evaluator's primitive-application rules.
    Ptr proc = v("proc"), arg = v("arg");
    Ptr proc_tail = cdr(proc);
    Ptr proc_params = car(proc_tail);
    Ptr proc_body = car(cdr(proc_tail));
    Ptr proc_env = car(cdr(cdr(proc_tail)));
    Ptr parameter = car(proc_params);
    Ptr new_binding = make_pair(parameter, arg);
    Ptr extended_env = make_pair(new_binding, proc_env);
    Ptr recursive_body = self_call(proc_body, extended_env);
    Ptr host_primitive_apply = scall2("apply", proc, make_pair(arg, sq(sym("vnil"))));
    Ptr apply_obj_body = sif(
        scall1("dataClosure?", proc),
        recursive_body,
        host_primitive_apply);
    Ptr apply_obj = slambda("proc", slambda("arg", apply_obj_body));

    Ptr source_expr = v("expr"), source_env = v("env");
    Ptr first_field = field(source_expr, 1);
    Ptr second_field = field(source_expr, 2);
    Ptr third_field = field(source_expr, 3);
    Ptr literal_case = first_field;
    Ptr quote_case = first_field;
    Ptr variable_case = cdr(scall2("assq", first_field, source_env));
    Ptr closure_value = make_pair(
        quoted_symbol("tag-closure"),
        make_pair(first_field,
            make_pair(second_field,
                make_pair(source_env, sq(sym("vnil"))))));
    Ptr conditional_case = sif(
        self_call(first_field, source_env),
        self_call(second_field, source_env),
        self_call(third_field, source_env));
    Ptr app_function = self_call(first_field, source_env);
    Ptr app_arg_expr = car(second_field); // application node stores an arg list
    Ptr app_argument = self_call(app_arg_expr, source_env);
    Ptr application_case = scall2("applyObj", app_function, app_argument);
    Ptr false_value = sq(sym("vfalse"));
    Ptr dispatch = sif(same_tag(source_expr, "tag-lit"), literal_case,
        sif(same_tag(source_expr, "tag-quote"), quote_case,
        sif(same_tag(source_expr, "tag-var"), variable_case,
        sif(same_tag(source_expr, "tag-lambda"), closure_value,
        sif(same_tag(source_expr, "tag-if"), conditional_case,
        sif(same_tag(source_expr, "tag-app"), application_case, false_value))))));
    Ptr evaluator_body = sapp(slambda("applyObj", slambda("expr", slambda("env", dispatch))), apply_obj);
    Ptr evaluator_functional = slambda("self", evaluator_body);

    // Call-by-value Z combinator. The self argument is delayed under a lambda,
    // so the recursive interpreter can be evaluated in Scheme's eager order.
    auto delayed_self = [&](const char* zname) {
        return slambda("v", sapp(sapp(v(zname), v(zname)), v("v")));
    };
    Ptr z_combinator = slambda("f",
        sapp(
            slambda("z", sapp(v("f"), delayed_self("z"))),
            slambda("z", sapp(v("f"), delayed_self("z")))));
    Ptr self_interpreter_source = sapp(z_combinator, evaluator_functional);

    // The source term is reified as Scheme data, then evaluated by the
    // interpreter represented by that source term.
    Ptr source_data = reify_scheme_expr(self_interpreter_source);
    Ptr builtin_names[] = {sym("car"), sym("cdr"), sym("cons"), sym("eq?"), sym("assq"), sym("apply"), sym("dataClosure?")};
    const char* primitive_names[] = {"car", "cdr", "cons", "eq?", "assq", "apply", "dataClosure?"};
    const char* primitive_tags[] = {"pcar", "pcdr", "pcons", "peq", "passq", "papply", "pdataClosure"};
    Ptr host_env = sym("empty");
    Ptr object_env = sym("vnil");
    for (int i = 6; i >= 0; --i) {
        Ptr prim = c("vprim", {sym(primitive_tags[i])});
        host_env = c("bind", {builtin_names[i], prim, host_env});
        object_env = c("vpair", {c("vpair", {dsym(primitive_names[i]), prim}), object_env});
    }
    Ptr run_self_source = sapp(sapp(self_interpreter_source, sq(source_data)), sq(object_env));
    Ptr self_interpreted = nf(apps(sym("eval"), run_self_source, host_env), rules);
    auto source_type = infer({}, self_interpreter_source, sig, rules);
    if (!source_type) std::println("Self interpreter source type error: {}", source_type.error());
    auto [boot_head, boot_args] = unroll(self_interpreted);
    bool boot_ok = std::get_if<Sym>(&boot_head->v) && std::get<Sym>(boot_head->v).name == "ok" && boot_args.size() == 1;
    Ptr interpreted_closure = boot_ok ? boot_args[0] : sym("vfalse");
    auto [closure_head, closure_args] = unroll(interpreted_closure);
    bool self_source_became_closure = std::get_if<Sym>(&closure_head->v) &&
        std::get<Sym>(closure_head->v).name == "vpair" && closure_args.size() == 2 &&
        conv(closure_args[0], dsym("tag-closure"), rules);
    assert(source_type && conv(*source_type, e, rules));
    assert(self_source_became_closure);

    // As a separate execution check, run the same source interpreter on the
    // identity program; its result should be the numeral seven.
    Ptr identity_data = reify_scheme_expr(expr);
    Ptr direct_interpreted_identity = nf(apps(sym("eval"), sapp(sapp(self_interpreter_source, sq(identity_data)), sq(object_env)), host_env), rules);
    Ptr expected_seven = c("ok", {c("vnat", {nat(7)})});
    assert(conv(direct_interpreted_identity, expected_seven, rules));
    std::println("λΠ source term type checks as Expr: {}", static_cast<bool>(source_type));
    std::println("Interpreter source interprets its own encoded program to a closure: {}", self_source_became_closure);
    std::println("Source-defined interpreter evaluates identity program to 7: {}", conv(direct_interpreted_identity, expected_seven, rules));

    // Run the self-interpreted closure on the identity program. Use head
    // normalization here because the closure captures the full recursive
    // interpreter source and normalizing that inert environment is expensive.
    Ptr quoted_identity_data = dlist({dsym("tag-quote"), identity_data});
    Ptr quoted_env_data = dlist({dsym("tag-quote"), object_env});
    Ptr quoted_interpreted_closure = dlist({dsym("tag-quote"), interpreted_closure});
    Ptr self_app_datum = dlist({dsym("tag-app"),
        dlist({dsym("tag-app"), quoted_interpreted_closure, dlist({quoted_identity_data})}),
        dlist({quoted_env_data})});
    Ptr via_self_source = whnf(apps(sym("eval"), sapp(sapp(self_interpreter_source, sq(self_app_datum)), sq(object_env)), host_env), rules);
    bool self_run_identity = conv(via_self_source, expected_seven, rules);
    std::println("Interpreted source then runs identity to 7: {}", self_run_identity);
    assert(self_run_identity);
}

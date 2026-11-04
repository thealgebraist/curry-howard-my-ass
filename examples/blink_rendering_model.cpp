#include "../src/lambda_pi.hpp"
#include <cassert>

using namespace lp;

static Ptr fn(Ptr a, Ptr b) { return arrow(std::move(a), std::move(b)); }
static Rule rule(Ptr lhs, Ptr rhs) { return {std::move(lhs), std::move(rhs)}; }

int main() {
    Signature sig;
    std::vector<Rule> rules;
    auto p = [](size_t i) { return pvar(i); };

    // A typed semantic pipeline following Blink core's broad rendering stages:
    // HTML input -> DOM -> styled DOM -> layout tree -> paint artifact.
    // These are model types: the constructors intentionally cover one tiny case.
    for (const char* type : {"Html", "Css", "Dom", "StyledDom", "LayoutTree", "PaintArtifact"})
        sig[type] = type_sort();

    sig["htmlP"] = fn(sym("Html"), sym("Html"));
    sig["htmlText"] = fn(sym("Text"), sym("Html"));
    sig["Text"] = type_sort();
    sig["hello"] = sym("Text");
    sig["pTag"] = sym("Tag"); sig["Tag"] = type_sort();
    sig["domElement"] = fn(sym("Tag"), fn(sym("Dom"), sym("Dom")));
    sig["domText"] = fn(sym("Text"), sym("Dom"));
    sig["cssP"] = fn(sym("Display"), fn(sym("Color"), sym("Css")));
    sig["Display"] = type_sort(); sig["Color"] = type_sort();
    sig["block"] = sym("Display"); sig["black"] = sym("Color");
    sig["styledElement"] = fn(sym("Tag"), fn(sym("Display"), fn(sym("Color"), fn(sym("StyledDom"), sym("StyledDom")))));
    sig["styledText"] = fn(sym("Text"), fn(sym("Color"), sym("StyledDom")));
    sig["blockBox"] = fn(sym("Color"), fn(sym("LayoutTree"), sym("LayoutTree")));
    sig["textRun"] = fn(sym("Text"), fn(sym("Color"), sym("LayoutTree")));
    sig["paintBlock"] = fn(sym("Color"), fn(sym("PaintArtifact"), sym("PaintArtifact")));
    sig["paintText"] = fn(sym("Text"), fn(sym("Color"), sym("PaintArtifact")));

    sig["parseHtml"] = fn(sym("Html"), sym("Dom"));
    sig["computeStyle"] = fn(sym("Dom"), fn(sym("Css"), sym("StyledDom")));
    sig["layoutDocument"] = fn(sym("StyledDom"), sym("LayoutTree"));
    sig["paintDocument"] = fn(sym("LayoutTree"), sym("PaintArtifact"));
    sig["renderPage"] = fn(sym("Html"), fn(sym("Css"), sym("PaintArtifact")));

    // Parsing and each subsequent phase are explicit rewrite steps.
    rules.push_back(rule(app(sym("parseHtml"), app(sym("htmlP"), app(sym("htmlText"), sym("hello")))),
                         apps(sym("domElement"), sym("pTag"), app(sym("domText"), sym("hello")))));
    rules.push_back(rule(apps(sym("computeStyle"),
                             apps(sym("domElement"), sym("pTag"), app(sym("domText"), sym("hello"))),
                             apps(sym("cssP"), sym("block"), sym("black"))),
                         apps(sym("styledElement"), sym("pTag"), sym("block"), sym("black"),
                              apps(sym("styledText"), sym("hello"), sym("black")))));
    rules.push_back(rule(app(sym("layoutDocument"),
                             apps(sym("styledElement"), sym("pTag"), sym("block"), sym("black"),
                                  apps(sym("styledText"), sym("hello"), sym("black")))),
                         apps(sym("blockBox"), sym("black"), apps(sym("textRun"), sym("hello"), sym("black")))));
    rules.push_back(rule(app(sym("paintDocument"),
                             apps(sym("blockBox"), sym("black"), apps(sym("textRun"), sym("hello"), sym("black")))),
                         apps(sym("paintBlock"), sym("black"), apps(sym("paintText"), sym("hello"), sym("black")))));
    rules.push_back(rule(apps(sym("renderPage"), p(0), p(1)),
                         app(sym("paintDocument"),
                             app(sym("layoutDocument"),
                                 apps(sym("computeStyle"), app(sym("parseHtml"), p(0)), p(1))))));

    Ptr html = app(sym("htmlP"), app(sym("htmlText"), sym("hello")));
    Ptr css = apps(sym("cssP"), sym("block"), sym("black"));
    Ptr render = apps(sym("renderPage"), html, css);
    Ptr result = nf(render, rules);
    Ptr expected = apps(sym("paintBlock"), sym("black"),
                        apps(sym("paintText"), sym("hello"), sym("black")));
    auto result_type = infer({}, render, sig, rules);

    assert(result_type && conv(*result_type, sym("PaintArtifact"), rules));
    assert(conv(result, expected, rules));
    std::println("Modeled Blink stages: HTML -> DOM -> styled DOM -> layout tree -> paint artifact");
    std::println("λΠ result: {}", to_string(result));
    std::println("Render term type checks: {}", static_cast<bool>(result_type));
}

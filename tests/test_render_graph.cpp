#include <doctest/doctest.h>
#include "test_common.hpp"

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

TEST_SUITE("RenderGraph") {

TEST_CASE("Empty render graph has no passes") {
    RenderGraph g;
    CHECK(g.passes().empty());
}

TEST_CASE("Add single pass") {
    RenderGraph g;
    g.add_pass({"gbuffer", 0});
    CHECK(g.passes().size() == 1);
    CHECK(g.passes()[0].name == "gbuffer");
    CHECK(g.passes()[0].priority == 0);
}

TEST_CASE("Add multiple passes with priorities") {
    RenderGraph g;
    g.add_pass({"skybox",    0});
    g.add_pass({"gbuffer",   1});
    g.add_pass({"lighting",  2});
    g.add_pass({"post",     99});

    CHECK(g.passes().size() == 4);
    CHECK(g.passes()[0].name == "skybox");
    CHECK(g.passes()[1].name == "gbuffer");
    CHECK(g.passes()[2].name == "lighting");
    CHECK(g.passes()[3].name == "post");

    CHECK(g.passes()[0].priority == 0);
    CHECK(g.passes()[3].priority == 99);
}

TEST_CASE("Pass ordering is insertion order") {
    RenderGraph g;
    g.add_pass({"A", 5});
    g.add_pass({"B", 1});
    g.add_pass({"C", 9});

    // Passes should be in insertion order (not sorted by priority)
    CHECK(g.passes()[0].name == "A");
    CHECK(g.passes()[1].name == "B");
    CHECK(g.passes()[2].name == "C");
}

} // TEST_SUITE("RenderGraph")

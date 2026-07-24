#include <doctest/doctest.h>
#include "test_common.hpp"
#include <exd/render/interaction/selection.hpp>
#include <exd/render/components/selected.hpp>
#include <exd/render/components/hovered.hpp>
#include <exd/render/components/transform.hpp>
#include <exd/render/components/renderable.hpp>

using namespace exd;
using namespace exd::render;
using namespace exd::render::test;

TEST_SUITE("SelectionSystem") {

TEST_CASE("Click on entity selects it") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e = reg.create("Entity");
    reg.emplace<Transform>(e);

    sel.handle_click(reg, e, false);
    CHECK(reg.has<Selected>(e));
}

TEST_CASE("Click on empty space deselects all") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    auto e2 = reg.create("B");
    reg.emplace<Transform>(e1);
    reg.emplace<Transform>(e2);
    reg.emplace<Selected>(e1);
    reg.emplace<Selected>(e2);

    sel.handle_click(reg, std::nullopt, false);

    CHECK(!reg.has<Selected>(e1));
    CHECK(!reg.has<Selected>(e2));
}

TEST_CASE("Click selects exactly one entity") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    auto e2 = reg.create("B");
    reg.emplace<Transform>(e1);
    reg.emplace<Transform>(e2);

    sel.handle_click(reg, e1, false);

    CHECK(reg.has<Selected>(e1));
    CHECK(!reg.has<Selected>(e2));
}

TEST_CASE("Second click deselects first and selects new") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    auto e2 = reg.create("B");
    reg.emplace<Transform>(e1);
    reg.emplace<Transform>(e2);

    sel.handle_click(reg, e1, false);
    CHECK(reg.has<Selected>(e1));

    sel.handle_click(reg, e2, false);
    CHECK(!reg.has<Selected>(e1));
    CHECK(reg.has<Selected>(e2));
}

TEST_CASE("Shift-click toggles selection") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    auto e2 = reg.create("B");
    reg.emplace<Transform>(e1);
    reg.emplace<Transform>(e2);

    // Select e1
    sel.handle_click(reg, e1, false);
    CHECK(reg.has<Selected>(e1));

    // Shift-click e2 adds to selection
    sel.handle_click(reg, e2, true);
    CHECK(reg.has<Selected>(e1));
    CHECK(reg.has<Selected>(e2));
}

TEST_CASE("Shift-click deselects already-selected entity") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    reg.emplace<Transform>(e1);
    reg.emplace<Selected>(e1);

    sel.handle_click(reg, e1, true);
    CHECK(!reg.has<Selected>(e1));
}

TEST_CASE("Hovered is placed on clicked entity") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e = reg.create("Entity");
    reg.emplace<Transform>(e);

    sel.handle_click(reg, e, false);
    CHECK(reg.has<Hovered>(e));
}

TEST_CASE("Hovered is removed on empty click") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e = reg.create("Entity");
    reg.emplace<Transform>(e);
    reg.emplace<Hovered>(e);

    sel.handle_click(reg, std::nullopt, false);
    CHECK(!reg.has<Hovered>(e));
}

TEST_CASE("clear_all removes Selected and Hovered") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    auto e1 = reg.create("A");
    auto e2 = reg.create("B");
    reg.emplace<Transform>(e1);
    reg.emplace<Transform>(e2);
    reg.emplace<Selected>(e1);
    reg.emplace<Hovered>(e1);
    reg.emplace<Selected>(e2);

    sel.clear_all(reg);

    CHECK(!reg.has<Selected>(e1));
    CHECK(!reg.has<Hovered>(e1));
    CHECK(!reg.has<Selected>(e2));
    CHECK(sel.selection_count(reg) == 0);
}

TEST_CASE("selection_count returns correct value") {
    exd::ecs::Registry reg;
    SelectionSystem sel;

    CHECK(sel.selection_count(reg) == 0);

    auto e = reg.create("Entity");
    reg.emplace<Transform>(e);
    sel.handle_click(reg, e, false);
    CHECK(sel.selection_count(reg) == 1);
}

} // TEST_SUITE("SelectionSystem")

#include <doctest.h>
#include <gta3sc/parser.hpp>
#include <cstring>
#include <iterator>
using namespace std::literals::string_view_literals;

namespace
{
template<std::size_t N>
auto make_source(const char (&data)[N]) -> gta3sc::SourceFile
{
    auto ptr = std::make_unique<char[]>(N);
    std::memcpy(ptr.get(), data, N);
    return gta3sc::SourceFile(std::move(ptr), N-1);
}

auto make_parser(const gta3sc::SourceFile& source, gta3sc::ArenaMemoryResource& arena)
    -> gta3sc::Parser
{
    auto pp = gta3sc::Preprocessor(source);
    auto scanner = gta3sc::Scanner(std::move(pp));
    return gta3sc::Parser(std::move(scanner), arena);
}
}

TEST_CASE("parsing a label definition")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("laBEL:\n"
                              "laBEL: WAIT 0\n"
                              "label:\n"
                              "WAIT 0\n"
                              "la:bel:\n"
                              "1abel:\n"
                              "lab\"el\":\n"
                              "\"label\":\n"
                              "lab\"el:\n"
                              ":\n"
                              "::\n"
                              "label:");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "LABEL");
    REQUIRE(ir->front().command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "LABEL");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().command->name == "WAIT");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "LA:BEL");

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1abel:
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el":
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "label":
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // lab"el:
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // :
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // ::
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "LABEL");
}

TEST_CASE("parsing a empty line")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("\n"
                              "WAIT 0\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->empty());

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
}

TEST_CASE("parsing a valid scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "WAIT 0\n"
                              "WAIT 1\n"
                              "}\n"
                              "WAIT 2\n"
                              "{\n"
                              "}\n"
                              "WAIT 3\n");
    auto parser = make_parser(source, arena);

    auto linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);

    auto ir = linked->begin();
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->args.size() == 0);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->args.size() == 0);
    REQUIRE(++ir == linked->end());

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();;
    REQUIRE(ir->command->name == "{");
    REQUIRE(ir->command->args.size() == 0);

    ++ir;
    REQUIRE(ir != linked->end());
    REQUIRE(ir->command->name == "}");
    REQUIRE(ir->command->args.size() == 0);
    REQUIRE(++ir == linked->end());

    linked = parser.parse_statement();
    REQUIRE(linked != std::nullopt);
    ir = linked->begin();
    REQUIRE(ir->command->name == "WAIT");
    REQUIRE(ir->command->args.size() == 1);
}

TEST_CASE("parsing a nested scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "{\n"
                              "}\n"
                              "}\n");
    auto parser = make_parser(source, arena);

    auto linked = parser.parse_statement();
    REQUIRE(linked == std::nullopt);
}

TEST_CASE("parsing a } outside a scope block")
{
    // TODO
    return;
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("}\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a unclosed scope block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a command")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("waIT 10 20 30\n"
                              "C\n"
                              "c\n"
                              "l: c:\n"
                              "a.sc\n"
                              "\"a\"\n"
                              "%\n"
                              "$\n"
                              "1\n"
                              ".1\n"
                              "-1\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 3);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "C");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "C");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().label->name == "L");
    REQUIRE(ir->front().command->name == "C:");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "A.SC");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a"
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "%");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "$");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "1");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == ".1");
    REQUIRE(ir->front().command->args.size() == 0);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "-1");
    REQUIRE(ir->front().command->args.size() == 0);
}

TEST_CASE("parsing integer argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT 123 010 -39\n"
                              "WAIT 2147483647 -2147483648\n"
                              "WAIT -432-10\n"
                              "WAIT 123a\n"
                              "WAIT 0x10\n"
                              "WAIT +39\n"
                              "WAIT 432+10\n"
                              "WAIT x -\n"
                              "WAIT x --\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 3);
    REQUIRE(*ir->front().command->args[0]->as_integer() == 123);
    REQUIRE(*ir->front().command->args[1]->as_integer() == 10);
    REQUIRE(*ir->front().command->args[2]->as_integer() == -39);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 2);
    REQUIRE(*ir->front().command->args[0]->as_integer()
            == std::numeric_limits<int32_t>::max());
    REQUIRE(*ir->front().command->args[1]->as_integer()
            == std::numeric_limits<int32_t>::min());

    ir = parser.parse_statement();
    parser.skip_current_line(); // -432-10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 123a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //0x10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // +39
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 432+10
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // -
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // --
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing float argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT .1 -.1 .1f .1F .15 .1.9 -.1.\n"
                              "WAIT 1F -1f 1. 1.1 1.f 1.. -1..\n"
                              "WAIT .1a\n"
                              "WAIT .1fa\n"
                              "WAIT .1.a\n"
                              "WAIT 1..a\n"
                              "WAIT .1-.1\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 7);
    REQUIRE(*ir->front().command->args[0]->as_float() == 0.1f);
    REQUIRE(*ir->front().command->args[1]->as_float() == -0.1f);
    REQUIRE(*ir->front().command->args[2]->as_float() == 0.1f);
    REQUIRE(*ir->front().command->args[3]->as_float() == 0.1f);
    REQUIRE(*ir->front().command->args[4]->as_float() == 0.15f);
    REQUIRE(*ir->front().command->args[5]->as_float() == 0.1f);
    REQUIRE(*ir->front().command->args[6]->as_float() == -0.1f);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 7);
    REQUIRE(*ir->front().command->args[0]->as_float() == 1.0f);
    REQUIRE(*ir->front().command->args[1]->as_float() == -1.0f);
    REQUIRE(*ir->front().command->args[2]->as_float() == 1.0f);
    REQUIRE(*ir->front().command->args[3]->as_float() == 1.1f);
    REQUIRE(*ir->front().command->args[4]->as_float() == 1.0f);
    REQUIRE(*ir->front().command->args[5]->as_float() == 1.0f);
    REQUIRE(*ir->front().command->args[6]->as_float() == -1.0f);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // .1a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1fa
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //.1.a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // 1..a
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // .1-.1
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing identifier argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT $abc abc AbC a@_1$\n"
                              "WAIT _abc\n"
                              "WAIT @abc\n"
                              "WAIT 1abc\n"
                              "WAIT abc: def\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 4);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "$ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[2]->as_identifier(), "ABC"sv);
    REQUIRE_EQ(*ir->front().command->args[3]->as_identifier(), "A@_1$"sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // _abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // @abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); //1abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // abc: def
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing string literal argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WAIT \"this\tI$ /* a // \\n (%1teral),\"\n"
                              "WAIT \"\"\n"
                              "WAIT \"\n"
                              "WAIT \"string\"abc\n"
                              "WAIT 9");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_string(),
               "this\tI$ /* a // \\n (%1teral),"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_string(), ""sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // "
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "string"abc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // 9
}

TEST_CASE("parsing filename argument")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("LAUNCH_MISSION .sc\n"
                              "LAUNCH_MISSION a.SC\n"
                              "WAIT a.SC\n"
                              "WAIT 1.SC\n"
                              "LAUNCH_MISSION @.sc\n"
                              "LAUNCH_MISSION 1.sc\n"
                              "LAUNCH_MISSION 1.0sc\n"
                              "LAUNCH_MISSION SC\n"
                              "LAUNCH_MISSION C\n"
                              "LAUNCH_MISSION \"a\".sc\n"
                              "LOAD_AND_LAUNCH_MISSION file-name.sc\n"
                              "GOSUB_FILE label file-name.sc\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), ".SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "A.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // WAIT a.SC

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt); // WAIT 1.sc
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "@.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "LAUNCH_MISSION");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "1.SC"sv);
    
    ir = parser.parse_statement();
    parser.skip_current_line(); // 1.0sc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // SC
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // C
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    parser.skip_current_line(); // "a".sc
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // LOAD_AND_LAUNCH_MISSION
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_filename(), "FILE-NAME.SC"sv);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt); // GOSUB_FILE
    REQUIRE(ir->front().command->args.size() == 2);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "LABEL"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_filename(), "FILE-NAME.SC"sv);
}

TEST_CASE("parsing permutations of absolute expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("x = aBs y\n"
                              "x = ABS x\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "SET");
    REQUIRE(it->command->args.size() == 2);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
    REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Y"sv);
    ++it;
    REQUIRE(it->command->name == "ABS");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
    REQUIRE(++it == ir->end());

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "ABS");
    REQUIRE(ir->front().command->args.size() == 1);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
    REQUIRE(ir->size() == 1);
}

TEST_CASE("parsing permutations of unary expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("++x\n"
                              "x++\n"
                              "--x\n"
                              "x--\n");
    auto parser = make_parser(source, arena);

    constexpr std::string_view expects_table[] = {
        "ADD_THING_TO_THING",
        "ADD_THING_TO_THING",
        "SUB_THING_FROM_THING",
        "SUB_THING_FROM_THING",
    };

    for(auto& command_name : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_integer(), 1);
        REQUIRE(ir->size() == 1);
    }
}

TEST_CASE("parsing permutations of binary expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("x = y\n"
                              "x = x\n"
                              "x =# y\n"
                              "x =# x\n"
                              "x += y\n"
                              "x += x\n"
                              "x -= y\n"
                              "x -= x\n"
                              "x *= y\n"
                              "x *= x\n"
                              "x /= y\n"
                              "x /= x\n"
                              "x +=@ y\n"
                              "x +=@ x\n"
                              "x -=@ y\n"
                              "x -=@ x\n");
    auto parser = make_parser(source, arena);

    constexpr std::string_view expects_table[] = {
        "SET",
        "CSET",
        "ADD_THING_TO_THING",
        "SUB_THING_FROM_THING",
        "MULT_THING_BY_THING",
        "DIV_THING_BY_THING",
        "ADD_THING_TO_THING_TIMED",
        "SUB_THING_FROM_THING_TIMED",
    };

    for(auto& command_name : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
        REQUIRE(ir->size() == 1);

        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "X"sv);
        REQUIRE(ir->size() == 1);
    }
}

TEST_CASE("parsing permutations of conditional expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x = y GOTO elsewhere\n"
                              "IFNOT x = x GOTO elsewhere\n"
                              "x < y\n"
                              "x < x\n"
                              "x <= y\n"
                              "x > y\n"
                              "x >= y\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 3);
    auto it = ir->begin();
    REQUIRE(std::next(it)->command->name == "IS_THING_EQUAL_TO_THING");

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 3);
    it = ir->begin();
    REQUIRE(std::next(it)->command->name == "IS_THING_EQUAL_TO_THING");

    constexpr std::array<std::string_view, 3> expects_table[] = {
        { "IS_THING_GREATER_THAN_THING", "Y", "X" },
        { "IS_THING_GREATER_THAN_THING", "X", "X" },
        { "IS_THING_GREATER_OR_EQUAL_TO_THING", "Y", "X" },
        { "IS_THING_GREATER_THAN_THING", "X", "Y" },
        { "IS_THING_GREATER_OR_EQUAL_TO_THING", "X", "Y" },
    };

    for(auto& [command_name, a, b] : expects_table)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), a);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), b);
        REQUIRE(ir->size() == 1);
    }
}

TEST_CASE("parsing permutations of ternary expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("x = x + x\n"
                              "x = x + y\n"
                              "x = y + x\n"
                              "x = y + z\n"
                              "x = x - x\n"
                              "x = x - y\n"
                              "x = y - x\n" // invalid
                              "x = y - z\n"
                              "x = x * x\n"
                              "x = x * y\n"
                              "x = y * x\n"
                              "x = y * z\n"
                              "x = x / x\n"
                              "x = x / y\n"
                              "x = y / x\n" // invalid
                              "x = y / z\n"
                              "x = x +@ x\n"
                              "x = x +@ y\n"
                              "x = y +@ x\n" // invalid
                              "x = y +@ z\n"
                              "x = x -@ x\n"
                              "x = x -@ y\n"
                              "x = y -@ x\n" // invalid
                              "x = y -@ z\n");
    auto parser = make_parser(source, arena);

    constexpr std::string_view expects_table[] = {
        "ADD_THING_TO_THING",
        "SUB_THING_FROM_THING",
        "MULT_THING_BY_THING",
        "DIV_THING_BY_THING",
        "ADD_THING_TO_THING_TIMED",
        "SUB_THING_FROM_THING_TIMED",
    };

    for(auto& command_name : expects_table)
    {
        // x = x + x
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "X"sv);
        REQUIRE(ir->size() == 1);

        // x = x + y
        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        REQUIRE(ir->front().command->name == command_name);
        REQUIRE(ir->front().command->args.size() == 2);
        REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
        REQUIRE(ir->size() == 1);

        // x = y + x
        if(command_name == "ADD_THING_TO_THING"
                || command_name == "MULT_THING_BY_THING")
        {
            ir = parser.parse_statement();
            REQUIRE(ir != std::nullopt);
            REQUIRE(ir->front().command->name == command_name);
            REQUIRE(ir->front().command->args.size() == 2);
            REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
            REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
            REQUIRE(ir->size() == 1);
        }
        else
        {
            ir = parser.parse_statement();
            REQUIRE(ir == std::nullopt);
            parser.skip_current_line();
        }

        // x = y + z
        ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
        auto it = ir->begin();
        REQUIRE(it->command->name == "SET");
        REQUIRE(it->command->args.size() == 2);
        REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Y"sv);
        REQUIRE((++it)->command->name == command_name);
        REQUIRE(it->command->args.size() == 2);
        REQUIRE_EQ(*it->command->args[0]->as_identifier(), "X"sv);
        REQUIRE_EQ(*it->command->args[1]->as_identifier(), "Z"sv);
        REQUIRE(++it == ir->end());
    }
}

TEST_CASE("parsing the ternary minus one ambiguity")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("x = 1-1\n"
                              "x = 1 -1\n"
                              "x = 1 - 1\n"
                              "x = 1--1\n"
                              "x = 1- -1\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
}

TEST_CASE("parsing operators not in expression")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("+= 1\n"
                              "x / 2\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();
}

TEST_CASE("parsing invalid expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("--x c\n"
                              "x++ c\n"
                              "x = ABS y z\n"
                              "x = y +\n"
                              "x = + y\n"
                              "x = y + z + w\n"
                              "x = y z\n"
                              "x += y + z\n"
                              "x =#\n"
                              "x < y + z\n"
                              "x <\n"
                              "x + y\n"
                              "x = y += z\n");
    auto parser = make_parser(source, arena);

    for(size_t count = 0; ; ++count)
    {
        auto ir = parser.parse_statement();
        if(ir != std::nullopt && ir->empty())
        {
            REQUIRE(count == 13);
            break;
        }
        REQUIRE(ir == std::nullopt);
        parser.skip_current_line();
    }
}

TEST_CASE("parsing expressions with no whitespaces")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("-- x\n"
                              "x ++\n"
                              "x=ABS y\n"
                              "x=y+z\n"
                              "x+=y\n"
                              "x<y\n"
                              "x<=y\n");
    auto parser = make_parser(source, arena);

    for(size_t count = 0; ; ++count)
    {
        auto ir = parser.parse_statement();
        if(ir != std::nullopt && ir->empty())
        {
            REQUIRE(count == 7);
            break;
        }
        REQUIRE(ir != std::nullopt);
    }
}

TEST_CASE("parsing commands with operators in the middle")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("COMMAND x - y\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing special words in expressions")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source(// The following is invalid:
                              "GOSUB_FILE++\n"
                              "++GOSUB_FILE\n"
                              "GOSUB_FILE ++\n"
                              "++ GOSUB_FILE\n"
                              "LAUNCH_MISSION ++\n"
                              "GOSUB_FILE = OTHER\n"
                              "LOAD_AND_LAUNCH_MISSION = OTHER\n"
                              "MISSION_START = OTHER\n"
                              "MISSION_END = OTHER\n"
                              "MISSION_START ++\n"
                              "MISSION_END ++\n"
                              "++ MISSION_START\n"
                              "++MISSION_START\n"
                              "++MISSION_END\n"
                              // The following is valid:
                              "OTHER = GOSUB_FILE\n"
                              "VAR_INT = LVAR_INT\n"
                              "WHILE = ENDWHILE\n"
                              "ENDIF = IF\n"
                              "ELSE = ENDIF\n"
                              "IFNOT = IFNOT\n"
                              "REPEAT = ENDREPEAT\n"
                              "ABS = ABS ABS\n");
    auto parser = make_parser(source, arena);

    for(int invalid = 14; invalid > 0; --invalid)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        parser.skip_current_line();
    }

    for(int valid = 8; valid > 0; --valid)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir != std::nullopt);
    }

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->empty());
}

TEST_CASE("parsing a valid IF...GOTO statement")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING GOTO elsewhere\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "GOTO_IF_TRUE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "ELSEWHERE"sv);
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid IFNOT...GOTO statement")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IFNOT SOMETHING GOTO elsewhere\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "GOTO_IF_FALSE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE_EQ(*it->command->args[0]->as_identifier(), "ELSEWHERE"sv);
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid conditional element with equal operator")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x = y GOTO elsewhere\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "ANDOR");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE((++it)->command->name == "GOTO_IF_TRUE");
    REQUIRE(it->command->args.size() == 1);
    REQUIRE(*it->command->args[0]->as_identifier() == "ELSEWHERE"sv);
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a ternary expression with a GOTO following it")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x = y + z GOTO elsewhere\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a conditional element with assignment expression")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x += y GOTO elsewhere\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a valid IF...ENDIF block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid IF...ELSE...ENDIF block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ELSE\n"
                              "    DO_3\n"
                              "    DO_4\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ELSE");
    REQUIRE((++it)->command->name == "DO_3");
    REQUIRE((++it)->command->name == "DO_4");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid IFNOT...ENDIF block")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IFNOT SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IFNOT");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid NOT")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF NOT SOMETHING\n"
                              "OR NOT OTHER_THING\n"
                              "OR ANOTHER_THING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 22);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a IF without ENDIF")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a IF...ELSE without ENDIF")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ELSE\n"
                              "    DO_3\n"
                              "    DO_4\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a ELSE/ENDIF with no IF")
{
    return; // TODO and actually test for all special words

    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("ENDIF\n"
                              "ELSE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();
}

TEST_CASE("parsing a conditionless IF")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF \n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a valid AND list")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "AND OTHER_THING\n"
                              "AND ANOTHER_THING\n"
                              "AND THING_4\n"
                              "AND THING_5\n"
                              "AND THING_6\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 5);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE((++it)->command->name == "THING_4");
    REQUIRE((++it)->command->name == "THING_5");
    REQUIRE((++it)->command->name == "THING_6");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid OR list")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "OR OTHER_THING\n"
                              "OR ANOTHER_THING\n"
                              "OR THING_4\n"
                              "OR THING_5\n"
                              "OR THING_6\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(*it->command->args[0]->as_integer() == 25);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE((++it)->command->name == "THING_4");
    REQUIRE((++it)->command->name == "THING_5");
    REQUIRE((++it)->command->name == "THING_6");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing AND/OR/NOT outside of condition")
{
    return; // TODO

    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("AND SOMETHING\n"
                              "OR OTHER_THING\n"
                              "NOT AAAA\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
    parser.skip_current_line();

    ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}
TEST_CASE("parsing mixed AND/OR")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "OR OTHER_THING\n"
                              "AND ANOTHER_THING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing too many AND/OR")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "OR OTHER_THING\n"
                              "OR ANOTHER_THING\n"
                              "OR THING_4\n"
                              "OR THING_5\n"
                              "OR THING_6\n"
                              "OR THING_7\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a conditionless AND/OR")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF SOMETHING\n"
                              "OR \n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing a valid WHILE...ENDWHILE")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid WHILENOT...ENDWHILE")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILENOT SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILENOT");
    REQUIRE(*it->command->args[0]->as_integer() == 0);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a valid WHILE...ENDWHILE with AND/OR/NOT")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE SOMETHING\n"
                              "AND OTHER_THING\n"
                              "AND NOT ANOTHER_THING\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(it->command->not_flag == false);
    REQUIRE(*it->command->args[0]->as_integer() == 2);
    REQUIRE(it->command->args.size() == 1);
    REQUIRE((++it)->command->name == "SOMETHING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "OTHER_THING");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ANOTHER_THING");
    REQUIRE(it->command->not_flag == true);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE(it->command->not_flag == false);
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(it->command->not_flag == false);
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a WHILE without ENDWHILE")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE SOMETHING\n"
                              "    DO_1\n"
                              "    DO_2\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing nested blocks with empty statement list")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE THING_1\n"
                              "    WHILE THING_2\n"
                              "    ENDWHILE\n"
                              "ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE((++it)->command->name == "THING_1");
    REQUIRE((++it)->command->name == "WHILE");
    REQUIRE((++it)->command->name == "THING_2");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing valid REPEAT...ENDREPEAT")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("REPEAT 5 var\n"
                              "    DO_1\n"
                              "    DO_2\n"
                              "ENDREPEAT\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    auto it = ir->begin();
    REQUIRE(it->command->name == "REPEAT");
    REQUIRE(it->command->args.size() == 2);
    REQUIRE(*it->command->args[0]->as_integer() == 5);
    REQUIRE_EQ(*it->command->args[1]->as_identifier(), "VAR"sv);
    REQUIRE((++it)->command->name == "DO_1");
    REQUIRE((++it)->command->name == "DO_2");
    REQUIRE((++it)->command->name == "ENDREPEAT");
    REQUIRE(++it == ir->end()); 
}

TEST_CASE("parsing a REPEAT without ENDREPEAT")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("REPEAT 5 var\n"
                              "    DO_1\n"
                              "    DO_2\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing valid var declaration commands")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("VAR_INT x y z\n"
                              "LVAR_INT x y z\n"
                              "VAR_FLOAT x y z\n"
                              "LVAR_FLOAT x y z\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);

    ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 1);
    REQUIRE(ir->front().command->name == "LVAR_FLOAT");
    REQUIRE(ir->front().command->args.size() == 3);
    REQUIRE_EQ(*ir->front().command->args[0]->as_identifier(), "X"sv);
    REQUIRE_EQ(*ir->front().command->args[1]->as_identifier(), "Y"sv);
    REQUIRE_EQ(*ir->front().command->args[2]->as_identifier(), "Z"sv);
}

TEST_CASE("parsing invalid use of special names")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("MISSION_END\n"
                              "MISSION_START\n"
                              "}\n"
                              "NOT\n"
                              "AND\n"
                              "OR\n"
                              "ELSE\n"
                              "ENDIF\n"
                              "ENDWHILE\n"
                              "ENDREPEAT\n"
                              "IF {\n"
                              "IF NOT NOT\n"
                              "IF AND\n"
                              "IF IF 0\n"
                              "IF IFNOT 0\n"
                              "IF WHILE 0\n"
                              "IF REPEAT 4 x\n"
                              "IF GOSUB_FILE a b.sc\n"
                              "IF LAUNCH_MISSION b.sc\n"
                              "IF LOAD_AND_LAUNCH_MISSION b.sc\n"
                              "IF MISSION_START\n"
                              "IF MISSION_END\n"
                              "WAIT 0\n"); // valid sync point
    auto parser = make_parser(source, arena);

    for(auto invalid = 0; invalid < 22; ++invalid)
    {
        auto ir = parser.parse_statement();
        REQUIRE(ir == std::nullopt);
        parser.skip_current_line();
    }

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->front().command->name == "WAIT");
}

TEST_CASE("parsing var decl while trying to match a special name")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE x = 0\n"
                              "VAR_INT y\n"
                              "ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
}

TEST_CASE("parsing weird closing blocks")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE x = 0\n"
                              "    IF y = 0\n"
                              "        WAIT 0\n"
                              "ENDWHILE\n"
                              "    ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing labels in AND/OR")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x = 0\n"
                              "label: AND y = 0\n"
                              "    WAIT 0\n"
                              "ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir == std::nullopt);
}

TEST_CASE("parsing labels in }")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("{\n"
                              "WAIT 0\n"
                              "label: }\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 3);
    
    auto it = ir->begin();
    REQUIRE(it->command->name == "{");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "}");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

TEST_CASE("parsing labels in ELSE/ENDIF")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("IF x = 0\n"
                              "    WAIT 0\n"
                              "lab1: ELSE\n"
                              "    WAIT 1\n"
                              "lab2: ENDIF\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 6);
    
    auto it = ir->begin();
    REQUIRE(it->command->name == "IF");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ELSE");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LAB1");
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDIF");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LAB2");
    REQUIRE(++it == ir->end());
}

TEST_CASE("parsing labels in ENDWHILE")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("WHILE x = 0\n"
                              "    WAIT 0\n"
                              "label: ENDWHILE\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 4);
    
    auto it = ir->begin();
    REQUIRE(it->command->name == "WHILE");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "IS_THING_EQUAL_TO_THING");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDWHILE");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

TEST_CASE("parsing labels in ENDREPEAT")
{
    gta3sc::ArenaMemoryResource arena;
    auto source = make_source("REPEAT 2 x\n"
                              "    WAIT 0\n"
                              "label: ENDREPEAT\n");
    auto parser = make_parser(source, arena);

    auto ir = parser.parse_statement();
    REQUIRE(ir != std::nullopt);
    REQUIRE(ir->size() == 3);
    
    auto it = ir->begin();
    REQUIRE(it->command->name == "REPEAT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "WAIT");
    REQUIRE(it->label == nullptr);
    REQUIRE((++it)->command->name == "ENDREPEAT");
    REQUIRE(it->label != nullptr);
    REQUIRE(it->label->name == "LABEL");
    REQUIRE(++it == ir->end());
}

// TODO test labels on each of the unexpected places (see updated specs)

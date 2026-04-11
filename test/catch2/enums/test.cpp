#include <catch2/catch_all.hpp>
#include <include/datalayer.h>

TEST_CASE("Test enums")
{
    SECTION("enums with auto id")
    {
        REQUIRE(static_cast<int>(Status::Booting) == 0);
        REQUIRE(static_cast<int>(Status::Starting) == 1);
        REQUIRE(static_cast<int>(Status::Finished) == 2);
    }

    SECTION("enums with defined values")
    {
        REQUIRE(static_cast<int>(ErrorStates::None) == 0);
        REQUIRE(static_cast<int>(ErrorStates::Info) == 1);
        REQUIRE(static_cast<int>(ErrorStates::Warn) == 4);
        REQUIRE(static_cast<int>(ErrorStates::Error) == 5);
        REQUIRE(static_cast<int>(ErrorStates::Critical) == 9);
        REQUIRE(static_cast<int>(ErrorStates::Fatal) == 10);
    }

    SECTION("enums defined types")
    {
        static_assert(std::is_same_v<std::underlying_type_t<Status>, uint8_t>);
        static_assert(std::is_same_v<std::underlying_type_t<ErrorStates>, uint32_t>);
    }

    SECTION("enum runtime underlying type cast")
    {
        REQUIRE(static_cast<uint8_t>(Status::Booting) == uint8_t{ 0 });
        REQUIRE(static_cast<uint8_t>(Status::Starting) == uint8_t{ 1 });
        REQUIRE(static_cast<uint8_t>(Status::Finished) == uint8_t{ 2 });
        REQUIRE(static_cast<uint32_t>(ErrorStates::None) == uint32_t{ 0 });
        REQUIRE(static_cast<uint32_t>(ErrorStates::Fatal) == uint32_t{ 10 });
    }

    SECTION("enum auto id values are consecutive from zero")
    {
        constexpr auto booting = static_cast<int>(Status::Booting);
        constexpr auto starting = static_cast<int>(Status::Starting);
        constexpr auto finished = static_cast<int>(Status::Finished);
        static_assert(starting == booting + 1);
        static_assert(finished == starting + 1);
    }

    SECTION("enum explicit values are non-consecutive and correct")
    {
        REQUIRE(static_cast<int>(ErrorStates::None) < static_cast<int>(ErrorStates::Info));
        REQUIRE(static_cast<int>(ErrorStates::Info) < static_cast<int>(ErrorStates::Warn));
        REQUIRE(static_cast<int>(ErrorStates::Warn) < static_cast<int>(ErrorStates::Fatal));
    }
}

#include <catch2/catch_all.hpp>
#include <datalayer.h>

TEST_CASE("Test enums with auto id", "[Enums]")
{
    REQUIRE(static_cast<int>(Status::Booting) == 0);
    REQUIRE(static_cast<int>(Status::Starting) == 1);
    REQUIRE(static_cast<int>(Status::Finished) == 2);
}

TEST_CASE("Test enums with defined values", "[Enums]")
{
    REQUIRE(static_cast<int>(ErrorStates::Info) == 1);
    REQUIRE(static_cast<int>(ErrorStates::Warn) == 4);
    REQUIRE(static_cast<int>(ErrorStates::Error) == 5);
    REQUIRE(static_cast<int>(ErrorStates::Critical) == 9);
    REQUIRE(static_cast<int>(ErrorStates::Fatal) == 10);
}

TEST_CASE("Test enums defined types", "[Enums]")
{
    static_assert(std::is_same_v<std::underlying_type_t<Status>, uint8_t>);
    static_assert(std::is_same_v<std::underlying_type_t<ErrorStates>, uint32_t>);
}

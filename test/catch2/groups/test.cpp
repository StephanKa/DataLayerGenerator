#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <datalayer.h>

TEST_CASE("Test default group name", "[Groups]")
{
    using namespace std::string_literals;
    REQUIRE(DefaultGroupInfo.name == "DefaultGroup"s);
}

TEST_CASE("Test default group id", "[Groups]") { REQUIRE(DefaultGroupInfo.baseId == 0x4000); }

TEST_CASE("Test default group version", "[Groups]")
{
    constexpr auto version = DefaultGroupInfo.version;
    REQUIRE(version.major == 1);
    REQUIRE(version.minor == 2);
    REQUIRE(version.build == 3);
}

TEST_CASE("Test default group persistence", "[Groups]") { REQUIRE(DefaultGroupInfo.persist == DataLayer::Persistance::None); }

TEST_CASE("Test cyclic group name", "[Groups]")
{
    using namespace std::string_literals;
    REQUIRE(DefaultCyclicGroupInfo.name == "DefaultCyclicGroup"s);
}

TEST_CASE("Test cyclic group id", "[Groups]") { REQUIRE(DefaultCyclicGroupInfo.baseId == 0x5000); }

TEST_CASE("Test cyclic group version", "[Groups]")
{
    constexpr auto version = DefaultCyclicGroupInfo.version;
    REQUIRE(version.major == 4);
    REQUIRE(version.minor == 5);
    REQUIRE(version.build == 6);
}

TEST_CASE("Test cyclic group persistence", "[Groups]") { REQUIRE(DefaultCyclicGroupInfo.persist == DataLayer::Persistance::Cyclic); }

TEST_CASE("Test on-write group name", "[Groups]")
{
    using namespace std::string_literals;
    REQUIRE(DefaultOnWriteGroupInfo.name == "DefaultOnWriteGroup"s);
}

TEST_CASE("Test on-write group id", "[Groups]") { REQUIRE(DefaultOnWriteGroupInfo.baseId == 0x6000); }

TEST_CASE("Test on-write group version", "[Groups]")
{
    constexpr auto version = DefaultOnWriteGroupInfo.version;
    REQUIRE(version.major == 7);
    REQUIRE(version.minor == 8);
    REQUIRE(version.build == 9);
}

TEST_CASE("Test on-write group persistence", "[Groups]") { REQUIRE(DefaultOnWriteGroupInfo.persist == DataLayer::Persistance::OnWrite); }

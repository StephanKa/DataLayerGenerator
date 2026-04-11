#include <catch2/catch_all.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <include/datalayer.h>

TEST_CASE("Test groups", "[Groups]")
{
    SECTION("default group name")
    {
        using namespace std::string_literals;
        REQUIRE(DefaultGroupInfo.name == "DefaultGroup"s);
    }

    SECTION("default group id")
    {
        REQUIRE(DefaultGroupInfo.baseId == 0x4000);
    }

    SECTION("default group version")
    {
        constexpr auto version = DefaultGroupInfo.version;
        REQUIRE(version.major == 1);
        REQUIRE(version.minor == 2);
        REQUIRE(version.build == 3);
    }

    SECTION("default group persistence")
    {
        REQUIRE(DefaultGroupInfo.persist == DataLayer::PersistenceType::None);
    }

    SECTION("cyclic group name")
    {
        using namespace std::string_literals;
        REQUIRE(DefaultCyclicGroupInfo.name == "DefaultCyclicGroup"s);
    }

    SECTION("cyclic group id", "[Groups]")
    {
        REQUIRE(DefaultCyclicGroupInfo.baseId == 0x5000);
    }

    SECTION("cyclic group version")
    {
        constexpr auto version = DefaultCyclicGroupInfo.version;
        REQUIRE(version.major == 4);
        REQUIRE(version.minor == 5);
        REQUIRE(version.build == 6);
    }

    SECTION("cyclic group persistence")
    {
        REQUIRE(DefaultCyclicGroupInfo.persist == DataLayer::PersistenceType::Cyclic);
    }

    SECTION("on-write group name")
    {
        using namespace std::string_literals;
        REQUIRE(DefaultOnWriteGroupInfo.name == "DefaultOnWriteGroup"s);
    }

    SECTION("on-write group id")
    {
        REQUIRE(DefaultOnWriteGroupInfo.baseId == 0x6000);
    }

    SECTION("on-write group version")
    {
        constexpr auto version = DefaultOnWriteGroupInfo.version;
        REQUIRE(version.major == 7);
        REQUIRE(version.minor == 8);
        REQUIRE(version.build == 9);
    }

    SECTION("on-write group persistence")
    {
        REQUIRE(DefaultOnWriteGroupInfo.persist == DataLayer::PersistenceType::OnWrite);
    }

    SECTION("default group allowUpgrade is true")
    {
        static_assert(DefaultGroupInfo.allowUpgrade == true);
    }

    SECTION("cyclic group allowUpgrade defaults to false")
    {
        static_assert(DefaultCyclicGroupInfo.allowUpgrade == false);
    }

    SECTION("on-write group allowUpgrade defaults to false")
    {
        static_assert(DefaultOnWriteGroupInfo.allowUpgrade == false);
    }

    SECTION("group base ids are distinct")
    {
        static_assert(DefaultGroupInfo.baseId != DefaultCyclicGroupInfo.baseId);
        static_assert(DefaultCyclicGroupInfo.baseId != DefaultOnWriteGroupInfo.baseId);
        static_assert(DefaultGroupInfo.baseId != DefaultOnWriteGroupInfo.baseId);
    }

    SECTION("group versions are independent")
    {
        constexpr auto v1 = DefaultGroupInfo.version;
        constexpr auto v2 = DefaultCyclicGroupInfo.version;
        constexpr auto v3 = DefaultOnWriteGroupInfo.version;
        REQUIRE(v1.major != v2.major);
        REQUIRE(v2.major != v3.major);
    }
}

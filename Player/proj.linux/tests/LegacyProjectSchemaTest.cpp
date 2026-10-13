#include "Data/LegacyProjectSchema.h"

#include <cstdio>
#include <cstdlib>

static void require(bool value, const char *message)
{
    if (!value) {
        std::fprintf(stderr, "%s\n", message);
        std::exit(1);
    }
}

static int hitBits(const rapidjson::Value &json)
{
    return agtk::data::legacy::objectGroupBits(json,
        "hitObjectGroupBit", "hitPlayerGroup", "hitEnemyGroup", 1, 2);
}

static int collisionBits(const rapidjson::Value &json)
{
    return agtk::data::legacy::objectGroupBits(json,
        "collideWithObjectGroupBit", "collideWithPlayerGroup", "collideWithEnemyGroup", 1, 2);
}

int main()
{
    using namespace agtk::data::legacy;
    // Minimal synthetic legacy records; no sample-game assets are included.
    rapidjson::Document old;
    old.Parse(R"({"type":1,"hitPlayerGroup":true,"hitEnemyGroup":true,
        "collideWithPlayerGroup":false,"collideWithEnemyGroup":true,"collideWithTile":true})");
    require(!old.HasParseError(), "invalid legacy fixture");
    require(objectGroup(old) == 1, "legacy enemy type was not preserved");
    require(hitBits(old) == 3, "legacy hit flags were not combined");
    require(collisionBits(old) == 2, "legacy enemy-only wall collision changed");
    require(tileGroupBits(old) == 1, "legacy floor collision was disabled");
    require(inputBufferFrames(old) == 0, "legacy input gained buffering");

    rapidjson::Document modern;
    modern.Parse(R"({"group":2,"type":0,"hitObjectGroupBit":4,
        "hitPlayerGroup":true,"hitEnemyGroup":true,"collideWithObjectGroupBit":0,
        "collideWithPlayerGroup":true,"collideWithEnemyGroup":true,
        "collideWithTileGroupBit":0,"collideWithTile":true,"acceptFrameCount":5})");
    require(!modern.HasParseError(), "invalid modern fixture");
    require(objectGroup(modern) == 2, "modern group lost precedence");
    require(hitBits(modern) == 4, "modern groups beyond player/enemy were lost");
    require(collisionBits(modern) == 0, "legacy flags reenabled disabled modern collision");
    require(tileGroupBits(modern) == 0, "legacy flag reenabled disabled modern floor collision");
    require(inputBufferFrames(modern) == 5, "modern input buffer was reset");

    rapidjson::Document disabled;
    disabled.Parse(R"({"type":0,"hitPlayerGroup":false,"hitEnemyGroup":false,
        "collideWithPlayerGroup":false,"collideWithEnemyGroup":false,"collideWithTile":false})");
    require(objectGroup(disabled) == 0 && hitBits(disabled) == 0
        && collisionBits(disabled) == 0 && tileGroupBits(disabled) == 0,
        "disabled legacy collisions were enabled");

    rapidjson::Document empty;
    empty.Parse("{}");
    require(hitBits(empty) == 0 && collisionBits(empty) == 0 && tileGroupBits(empty) == 0,
        "missing collision flags enabled collisions");
    std::puts("legacy project schema tests passed");
}

#pragma once

#include "json/document.h"

namespace agtk { namespace data { namespace legacy {

inline int inputBufferFrames(const rapidjson::Value &json)
{
    // Old input conditions had no buffering beyond the current frame.
    return json.HasMember("acceptFrameCount") ? json["acceptFrameCount"].GetInt() : 0;
}

inline int objectGroup(const rapidjson::Value &json)
{
    return json.HasMember("group") ? json["group"].GetInt() : json["type"].GetInt();
}

inline int objectGroupBits(const rapidjson::Value &json, const char *mask,
    const char *player, const char *enemy, int playerBit, int enemyBit)
{
    // An explicit modern mask, including zero, takes precedence over old flags.
    if (json.HasMember(mask)) return json[mask].GetInt();
    int bits = 0;
    if (json.HasMember(player) && json[player].GetBool()) bits |= playerBit;
    if (json.HasMember(enemy) && json[enemy].GetBool()) bits |= enemyBit;
    return bits;
}

inline int tileGroupBits(const rapidjson::Value &json)
{
    // Legacy tiles all belong to the default tile group (bit zero).
    return json.HasMember("collideWithTileGroupBit")
        ? json["collideWithTileGroupBit"].GetInt()
        : (json.HasMember("collideWithTile") && json["collideWithTile"].GetBool() ? 1 : 0);
}

}}}

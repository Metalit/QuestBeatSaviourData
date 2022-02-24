#include "config.hpp"

DESERIALIZE_METHOD(BeatSaviorData, Config,
    DESERIALIZE_VALUE_DEFAULT(ShowOnPass, ShowPass, Bool, true);
    DESERIALIZE_VALUE_DEFAULT(ShowOnFail, ShowFail, Bool, true);
    DESERIALIZE_VALUE_DEFAULT(ShowGraph, ShowGraph, Bool, true);
    DESERIALIZE_VALUE_DEFAULT(NarrowGraphRange, GraphRange, Bool, true);
    DESERIALIZE_VALUE_DEFAULT(SaveLocally, Save, Bool, true);
    DESERIALIZE_VALUE_DEFAULT(UseCommas, Commas, Bool, false);
    DESERIALIZE_VALUE_DEFAULT(NumDecimals, Decimals, Int, 1);
)

SERIALIZE_METHOD(BeatSaviorData, Config,
    SERIALIZE_VALUE(ShowOnPass, ShowPass);
    SERIALIZE_VALUE(ShowOnFail, ShowFail);
    SERIALIZE_VALUE(ShowGraph, ShowGraph);
    SERIALIZE_VALUE(NarrowGraphRange, GraphRange);
    SERIALIZE_VALUE(SaveLocally, Save);
    SERIALIZE_VALUE(UseCommas, Commas);
    SERIALIZE_VALUE(NumDecimals, Decimals);
)

BeatSaviorData::Config globalConfig{};

#pragma once

#include "TypeMacros.hpp"

DECLARE_JSON_CLASS(BeatSaviorData, Config,
    bool ShowOnPass, ShowOnFail;
    bool ShowGraph, NarrowGraphRange;
    bool SaveLocally;
    bool UseCommas;
    int NumDecimals;
)

extern BeatSaviorData::Config globalConfig;
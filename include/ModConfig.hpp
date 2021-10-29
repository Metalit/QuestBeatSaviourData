#pragma once
#include "extern/config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(ShowPass, bool, "Show On Passes", true, "Show the stats after passing a level");
    CONFIG_VALUE(ShowFail, bool, "Show On Fails", true, "Show the stats after failing a level");
    CONFIG_VALUE(ShowGraph, bool, "Show Graph", true, "Show a performance graph");
    CONFIG_VALUE(GraphRange, bool, "Narrow Graph Range", true, "Set the percent at the bottom of the graph higher than 0");
    CONFIG_VALUE(Decimals, int, "Decimal Precision", 1, "The decimal precision to show");
    CONFIG_VALUE(Commas, bool, "Use Commas", false, "Commas instead of decimal points. But why??");
    CONFIG_VALUE(Save, bool, "Save Data Locally", true, "Store all results information in a local file");

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(ShowPass);
        CONFIG_INIT_VALUE(ShowFail);
        CONFIG_INIT_VALUE(ShowGraph);
        CONFIG_INIT_VALUE(GraphRange);
        CONFIG_INIT_VALUE(Decimals);
        CONFIG_INIT_VALUE(Commas);
        CONFIG_INIT_VALUE(Save);
    )
)
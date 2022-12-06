#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(Config,
    CONFIG_VALUE(ShowOnPass, bool, "Show On Passes", true, "Show data in the results screen on passing")
    CONFIG_VALUE(ShowOnFail, bool, "Show On Fails", true, "Show data in the results screen on failing")
    CONFIG_VALUE(ShowGraph, bool, "Show Graph", true, "Show the performance graph in the results screen")
    CONFIG_VALUE(NarrowGraphRange, bool, "Fit Graph", true, "Fit the performance graph to the variance of the particular score, instead of 0-100")
    CONFIG_VALUE(NumDecimals, int, "Decimal Precision", 1, "Number of decimal places to display")
    CONFIG_VALUE(SaveLocally, bool, "Save Locally", true, "Save data locally to be viewable after exiting the results screen")
    CONFIG_VALUE(UseCommas, bool, "Use Commas", false, "Put commas where decimal points should be")
)

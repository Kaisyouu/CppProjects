#pragma once
#include <iostream>
#include <unordered_map>
#include <map>
#include <list>
#include <vector>

typedef int OrderID;
typedef int Qty;
enum Side : uint8_t{
    kBuy = 0,
    kSell,
    kSideMax
};
enum MatchMode : uint8_t{
    kCentralized = 0,
    kOrderTrigger,
    kMatchModeMax
};


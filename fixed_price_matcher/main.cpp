#include <iostream>
#include "zrt_matcher.hpp"

class ZRTMatchHandler : public IMatchHandler {
    void OnMatch(MatchInfo* info) override {
        std::cout << "ZRT match trade confirm:" << std::endl
        << "trade_qty="<< info->GetTradeQty() << ", "
        << "buyer="<< info->GetBuyerOrderID() << ", "
        << "seller="<< info->GetSellerOrderID() << ", "
        << "buyer_sts="<< static_cast<uint8_t>(info->GetBuyerMatchStatus()) << ", "
        << "seller_sts="<< static_cast<uint8_t>(info->GetSellerMatchStatus()) << ", "
        << "buyer_left="<< info->GetBuyerLeaveQty() << ", "
        << "seller_left="<< info->GetSellerLeaveQty() << std::endl;
    }
};

void TestCase1() {
    ZRTMatchHandler* handler = new ZRTMatchHandler();
    ZRTMatcher* matcher= new ZRTMatcher();
    matcher->SetMatchHandler(handler);
    ZRTCtrlKey inst_key_1("10001", "ZRT", "", 2000, 15);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 1, 1000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 2, 2000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 3, 3000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 4, 4000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 5, 5000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 6, 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 7, 3000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 8, 4000);
    matcher->ZRTMatch();
    if (nullptr != handler) delete handler;
    if (nullptr != matcher) delete matcher;
}

void TestCase2() {
    ZRTMatchHandler* handler = new ZRTMatchHandler();
    ZRTMatcher* matcher= new ZRTMatcher();
    matcher->SetMatchHandler(handler);
    ZRTCtrlKey inst_key_1("10001", "ZRT", "", 2000, 15);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 1, 1000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 2, 2000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 3, 3000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 4, 4000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kBuy, 5, 5000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 6, 1000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 7, 3000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 8, 4000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 9, 4000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 10, 4000 * 1000);
    matcher->AddDelegation(inst_key_1, Side::kSell, 11, 5000 * 1000);
    matcher->ZRTMatch();
    if (nullptr != handler) delete handler;
    if (nullptr != matcher) delete matcher;
}

int main() {
    std::cout << "-----TestCase1 Start: Buy Qty > Sell Qty-----\n";
    TestCase1();
    std::cout << "-----TestCase1 Finish-----\n";
    std::cout << "-----TestCase2 Start: Buy Qty > Sell Qty-----\n";
    TestCase2();
    std::cout << "-----TestCase2 Finish-----\n";
}
#pragma once
#include <iostream>
#include <unordered_map>
#include <map>
#include <list>
#include <vector>
#include <functional>

#include "utils.hpp"

#define APP_QTY_DECIMAL 1000
#define ZRTC_INST_ID_LEN 6
#define ZRTC_BIZ_TYP_LEN 3
#define STR_FILL7_LEN 7
#define ZRT_SHORT_NAME 8
#define ZRT_MATCH_RATE_LEN 5
#define ZRT_QTY_BASE_VAL (100 * APP_QTY_DECIMAL)


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
/// @brief 转融通控制区主键 primKeyZrtInstCtrlTag
struct ZRTCtrlKey {
    char instId[ZRTC_INST_ID_LEN];
    char bizTyp[ZRTC_BIZ_TYP_LEN];
    char fill7[STR_FILL7_LEN];
    uint64_t price;
    uint64_t noDates;

    ZRTCtrlKey() {}
    ZRTCtrlKey(const std::string& inst_id,
               const std::string& biz_typ,
               const std::string& fill,
               const uint64_t px,
               const uint64_t no_dates) {
        memcpy(instId, inst_id.c_str(), inst_id.length());
        memcpy(bizTyp, biz_typ.c_str(), biz_typ.length());
        memcpy(fill7, biz_typ.c_str(), fill.length());
        price = px;
        noDates = no_dates;
    }
    
    bool operator==(const ZRTCtrlKey& other) const {
        return (std::memcmp(instId, other.instId, sizeof(instId)) == 0) &&
               (std::memcmp(bizTyp, other.bizTyp, sizeof(bizTyp)) == 0) &&
               (std::memcmp(fill7, other.fill7, sizeof(fill7)) == 0) &&
               (price == other.price) &&
               (noDates == other.noDates);
    }
};

/// @brief 转融通控制区主键Hash
struct ZRTCtrlKeyHash {
    std::size_t operator()(const ZRTCtrlKey& key) const {
        std::size_t hashValue = 0;
        hashValue ^= std::hash<uint64_t>()(key.price);
        hashValue ^= std::hash<uint64_t>()(key.noDates);
        hashValue ^= std::hash<std::string>()(key.instId);
        hashValue ^= std::hash<std::string>()(key.bizTyp);
        hashValue ^= std::hash<std::string>()(key.fill7);
        return hashValue;
    }
};

/// @brief 转融通产品控制信息 zrtinstctrlComTag
struct ZRTInstCtrl {
    // char shortName[ZRT_SHORT_NAME];
    int64_t sellQty;
    int64_t buyQty;
    int64_t trdQty;
};

/// @brief 时间链表
using TimeSeriesList = std::list<OrderID>;
/// @brief 链表索引
using TimeListIndexMap = std::unordered_map<OrderID, std::list<OrderID>::iterator>;
using QtyListIndexMap = std::unordered_map<OrderID, std::list<OrderID>::iterator>;
/// @brief 数量链表，按照数量倒序存储
struct QtyCompareKey {
    bool operator()(const Qty& a, const Qty& b) const {
        return a > b;
    }
};
using QtyTimeSeriesList = std::map<Qty, TimeSeriesList, QtyCompareKey>;
/// @brief 产品控制信息表
using ZRTInstCtrlMap = std::unordered_map<ZRTCtrlKey, ZRTInstCtrl, ZRTCtrlKeyHash>;
/// @brief 序列结构（时间序+数量序）
struct SeriesInfo {
    TimeSeriesList timeSeriesList;
    QtyTimeSeriesList qtyTimeSeriesList;
};
/// @brief 控制值
struct CtrlValue {
    ZRTInstCtrl instCtrl;
    SeriesInfo buyerSeries;
    SeriesInfo sellerSeries;
};
using ZRTCtrlMap = std::unordered_map<ZRTCtrlKey, CtrlValue, ZRTCtrlKeyHash>;
/// @brief 订单撮合状态
enum MatchStatus : uint8_t {
    kInit = 0,
    kPartial,
    kAllDeal,
    kDelete,
    kMatchStatusMax
};
/// @brief 订单信息
struct OrderInfo {
    Qty leftover;
    Qty matchQty;
    MatchStatus matchStatus;
};
using OrderInfoMap = std::unordered_map<OrderID, OrderInfo>;

class MatchInfo {
    friend class ZRTMatcher;
public:
    Qty GetTradeQty() { return trade_qty_; }
    OrderID GetBuyerOrderID() { return buy_ord_id_; }
    OrderID GetSellerOrderID() { return sell_ord_id_; }
    MatchStatus GetBuyerMatchStatus() { return buy_match_status_; }
    MatchStatus GetSellerMatchStatus() { return sell_match_status_; }
    Qty GetBuyerLeaveQty() { return buy_leave_qty_; }
    Qty GetSellerLeaveQty() { return sell_leave_qty_; }
protected:
    MatchInfo() = default;
    virtual ~MatchInfo() = default;
private:
    Qty trade_qty_;
    OrderID buy_ord_id_;
    OrderID sell_ord_id_;
    MatchStatus buy_match_status_;
    MatchStatus sell_match_status_;
    Qty buy_leave_qty_;
    Qty sell_leave_qty_;
};

class IMatchHandler {
public:
    virtual ~IMatchHandler() = default;
    virtual void OnMatch(MatchInfo* info) = 0;
};

class ZRTMatcher : public IMatchHandler{
public:
    ZRTMatcher() : m_match_handler(nullptr) {}
    ~ZRTMatcher() {}
    std::error_code AddDelegation(const ZRTCtrlKey& key,
                                  const Side side,
                                  const OrderID ord_id,
                                  const Qty ord_qty);
    std::error_code DelDelegation(const ZRTCtrlKey& key,
                                  const Side side,
                                  const OrderID ord_id);
    void ZRTMatch();
    void SetMatchHandler(IMatchHandler* handler) {
        m_match_handler = handler;
    }
private:
    void MatchByTime(const ZRTCtrlKey& key, CtrlValue& value);
    void MatchByQtyTime(const ZRTCtrlKey& key, CtrlValue& value);
    void ResetSellMatchQty(const ZRTCtrlKey& key, CtrlValue& value);
    void MatchQueryByTime(const OrderID buy_ord_id,
                          const OrderID sell_ord_id,
                          std::list<OrderID>::iterator& buy_it,
                          std::list<OrderID>::iterator& sell_it,
                          Qty* iQty);
    void MakeTradeRecord(const OrderID buy_ord_id,
                         const OrderID sell_ord_id,
                         const Qty iQty);
    void CalSellMatchQtyByProportion(const OrderID sell_ord_id,
                                     const double iRate,
                                     Qty* iSellQty);
    void CalSellMatchQtyByQtyTime(const OrderID sell_ord_id,
                                  Qty* iBuyQty);
    void CheckSetMatchStatus(const OrderID ord_id);
    void OnMatch(MatchInfo* info) override {}

private:
    ZRTCtrlMap m_map;
    TimeListIndexMap m_time_index_map;
    QtyListIndexMap m_qty_index_map;
    OrderInfoMap m_ord_info_map;
    IMatchHandler* m_match_handler;
};

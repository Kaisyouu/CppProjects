#include "zrt_matcher.hpp"

std::error_code ZRTMatcher::AddDelegation(const ZRTCtrlKey& key, const Side side, const OrderID ord_id, const Qty ord_qty) {
    if (side != Side::kBuy && side != Side::kSell) {
        return {};  // TechError::kInvalidSide
    }
    bool is_buy = side == Side::kBuy;
    auto iter = m_map.find(key);
    if (iter == m_map.end()) {
        CtrlValue value;
        value.instCtrl.buyQty = 0;
        value.instCtrl.sellQty = 0;
        value.instCtrl.trdQty = 0;
        if (is_buy) {
            value.buyerSeries.timeSeriesList.push_back(ord_id);
            m_time_index_map[ord_id] = std::prev(value.buyerSeries.timeSeriesList.end());
            value.buyerSeries.qtyTimeSeriesList[ord_qty].push_back(ord_id);
            m_qty_index_map[ord_id] = std::prev(value.buyerSeries.qtyTimeSeriesList[ord_qty].end());
            value.instCtrl.buyQty += ord_qty;
        }
        else {
            value.sellerSeries.timeSeriesList.push_back(ord_id);
            value.sellerSeries.qtyTimeSeriesList[ord_qty].push_back(ord_id);
            m_time_index_map[ord_id] = std::prev(value.sellerSeries.timeSeriesList.end());
            m_qty_index_map[ord_id] = std::prev(value.sellerSeries.qtyTimeSeriesList[ord_qty].end());
            value.instCtrl.sellQty += ord_qty;
        }
        m_ord_info_map.insert({ord_id, {ord_qty, ord_qty, MatchStatus::kInit}});
        m_map.insert({key, value});
    }
    else {
        if (is_buy) {
            iter->second.instCtrl.buyQty += ord_qty;
            iter->second.buyerSeries.timeSeriesList.push_back(ord_id);
            m_time_index_map[ord_id] = std::prev(iter->second.buyerSeries.timeSeriesList.end());
            iter->second.buyerSeries.qtyTimeSeriesList[ord_qty].push_back(ord_id);
            m_qty_index_map[ord_id] = std::prev(iter->second.buyerSeries.qtyTimeSeriesList[ord_qty].end());
        }
        else {
            iter->second.instCtrl.sellQty += ord_qty;
            iter->second.sellerSeries.timeSeriesList.push_back(ord_id);
            m_time_index_map[ord_id] = std::prev(iter->second.sellerSeries.timeSeriesList.end());
            iter->second.sellerSeries.qtyTimeSeriesList[ord_qty].push_back(ord_id);
            m_qty_index_map[ord_id] = std::prev(iter->second.sellerSeries.qtyTimeSeriesList[ord_qty].end());
        }
        m_ord_info_map.insert({ord_id, {ord_qty, ord_qty, MatchStatus::kInit}});
    }
    return {};  // TechError::kOK
}

std::error_code ZRTMatcher::DelDelegation(const ZRTCtrlKey& key, const Side side, const OrderID ord_id) {
    if (side != Side::kBuy && side != Side::kSell) {
        return {};  // TechError::kInvalidSide
    }
    bool is_buy = side == Side::kBuy;
    auto iter = m_map.find(key);
    if (iter == m_map.end()) {
        return {};  // TechError::kNotFindRec
    }
    auto tm_iter = m_time_index_map.find(ord_id);
    if (tm_iter == m_time_index_map.end()) {
        return {};  // TechError::kNotFindRec
    }
    auto qty_iter = m_qty_index_map.find(ord_id);
    if (qty_iter == m_qty_index_map.end()) {
        return {};  // TechError::kNotFindRec
    }

    if (is_buy) {
        iter->second.buyerSeries.timeSeriesList.erase(tm_iter->second);
        iter->second.buyerSeries.qtyTimeSeriesList[qty_iter->first].erase(qty_iter->second);
        iter->second.instCtrl.buyQty -= qty_iter->first;
    }
    else {
        iter->second.sellerSeries.timeSeriesList.erase(tm_iter->second);
        iter->second.sellerSeries.qtyTimeSeriesList[qty_iter->first].erase(qty_iter->second);
        iter->second.instCtrl.sellQty -= qty_iter->first;
    }
    if (m_ord_info_map.find(ord_id) != m_ord_info_map.end()) {
        m_ord_info_map.erase(ord_id);
    }
    return {};  // TechError::kOK;
}

void ZRTMatcher::ZRTMatch() {
    for (auto& iter : m_map) {
        if (iter.second.instCtrl.buyQty > iter.second.instCtrl.sellQty) {
            MatchByTime(iter.first, iter.second);
        }
        else {
            MatchByQtyTime(iter.first, iter.second);
        }
    }
}

void ZRTMatcher::MatchByQtyTime(const ZRTCtrlKey& key, CtrlValue& value) {
    // 计算卖方撮合金额
    ResetSellMatchQty(key, value);
    // 撮合逻辑
    MatchByTime(key, value);
}

/// @brief MatchChronological 按照时间优先规则进行撮合
/// @ref   ZRTProcssZiiOrdMatch
/// @param key 
/// @param value 
void ZRTMatcher::MatchByTime(const ZRTCtrlKey& key, CtrlValue& value) {
    // 撮合逻辑
    OrderID iBuyOrdNo = 0;
    OrderID iSellOrdNo = 0;
    Qty iQty = 0;
    auto buy_it = value.buyerSeries.timeSeriesList.begin();
    auto sell_it = value.sellerSeries.timeSeriesList.begin();
    while (1) {
        if (buy_it == value.buyerSeries.timeSeriesList.end() ||
            sell_it == value.sellerSeries.timeSeriesList.end()) {
            break;
        }
        iBuyOrdNo = *buy_it;
        iSellOrdNo = *sell_it;
        iQty = 0;
        MatchQueryByTime(iBuyOrdNo, iSellOrdNo, buy_it, sell_it, &iQty);
        if (iQty > 0) {
            // 成交逻辑
            value.instCtrl.trdQty += iQty;
            MakeTradeRecord(iBuyOrdNo, iSellOrdNo, iQty);
        }
    }
}

void ZRTMatcher::MatchQueryByTime(const OrderID buy_ord_id,
                                  const OrderID sell_ord_id,
                                  std::list<OrderID>::iterator& buy_it,
                                  std::list<OrderID>::iterator& sell_it,
                                  Qty* iQty) {
    // 买卖订单不是报价或部分成交，取下一条记录
    if (m_ord_info_map[buy_ord_id].matchStatus != MatchStatus::kInit &&
        m_ord_info_map[buy_ord_id].matchStatus != MatchStatus::kPartial) {
        buy_it++;
        return;
    }
    if (m_ord_info_map[sell_ord_id].matchStatus != MatchStatus::kInit &&
        m_ord_info_map[sell_ord_id].matchStatus != MatchStatus::kPartial) {
        sell_it++;
        return;
    }
    // 链表移动逻辑，卖方扣减数量=撮合数量
    // 买撮合数量>卖撮合数量，取下一个卖方
    if (m_ord_info_map[buy_ord_id].matchQty > m_ord_info_map[sell_ord_id].matchQty) {
        // 双方剩余都扣较小的撮合数量
        m_ord_info_map[buy_ord_id].leftover -= m_ord_info_map[sell_ord_id].matchQty;
        m_ord_info_map[sell_ord_id].leftover -= m_ord_info_map[sell_ord_id].matchQty;
        // 卖方的撮合全完成，撮合数量为0，买方相应扣减
        m_ord_info_map[buy_ord_id].matchQty -= m_ord_info_map[sell_ord_id].matchQty;
        // 产生本次撮合数量
        *iQty = m_ord_info_map[sell_ord_id].matchQty;
        m_ord_info_map[sell_ord_id].matchQty = 0;
        sell_it++;
    }
    // 买数量<卖数量，取下一个买方
    else if (m_ord_info_map[buy_ord_id].matchQty < m_ord_info_map[sell_ord_id].matchQty) {
        m_ord_info_map[buy_ord_id].leftover -= m_ord_info_map[buy_ord_id].matchQty;
        m_ord_info_map[sell_ord_id].leftover -= m_ord_info_map[buy_ord_id].matchQty;
        *iQty = m_ord_info_map[buy_ord_id].matchQty;
        m_ord_info_map[sell_ord_id].matchQty -= m_ord_info_map[buy_ord_id].matchQty;
        m_ord_info_map[buy_ord_id].matchQty = 0;
        buy_it++;
    }
    // 数量相等，同时取下一个
    else {
        m_ord_info_map[buy_ord_id].leftover -= m_ord_info_map[buy_ord_id].matchQty;
        m_ord_info_map[sell_ord_id].leftover -= m_ord_info_map[buy_ord_id].matchQty;
        *iQty = m_ord_info_map[buy_ord_id].matchQty;
        m_ord_info_map[buy_ord_id].matchQty = 0;
        m_ord_info_map[sell_ord_id].matchQty = 0;
        buy_it++;
        sell_it++;
    }
    // 有余量则部分成交，没有则全部成交
    CheckSetMatchStatus(buy_ord_id);
    CheckSetMatchStatus(sell_ord_id);
}

void ZRTMatcher::MakeTradeRecord(const OrderID buy_ord_id,
                                 const OrderID sell_ord_id,
                                 const Qty iQty) {
    // 成交逻辑
    if (nullptr == m_match_handler) {
        return;  // may be exception
    }
    MatchInfo* match_info = new MatchInfo();
    match_info->trade_qty_ = iQty;
    match_info->buy_ord_id_ = buy_ord_id;
    match_info->buy_leave_qty_ = m_ord_info_map[buy_ord_id].leftover;
    match_info->buy_match_status_ = m_ord_info_map[buy_ord_id].matchStatus;
    match_info->sell_ord_id_ = sell_ord_id;
    match_info->sell_leave_qty_ = m_ord_info_map[sell_ord_id].leftover;
    match_info->sell_match_status_ = m_ord_info_map[sell_ord_id].matchStatus;
    m_match_handler->OnMatch(match_info);
    delete match_info;
}

void ZRTMatcher::ResetSellMatchQty(const ZRTCtrlKey& key, CtrlValue& value) {
    double iRate = 0;
    OrderID iSellOrdNo = 0;
    Qty iSellMatchQty = 0;
    Qty iBuyMatchQty = 0;
    auto sell_it = value.sellerSeries.timeSeriesList.begin();
    iRate = DIVD(value.instCtrl.buyQty, value.instCtrl.sellQty);
    // 遍历一遍卖方时间链，按比例计算第一次撮合金额
    while (1) {
        iSellOrdNo = *sell_it;
        if (sell_it == value.sellerSeries.timeSeriesList.end()) {
            break;
        }
        if (m_ord_info_map[iSellOrdNo].matchStatus != MatchStatus::kInit &&
            m_ord_info_map[iSellOrdNo].matchStatus != MatchStatus::kAllDeal) {
            sell_it++;
            continue;
        }
        CalSellMatchQtyByProportion(iSellOrdNo, iRate, &iSellMatchQty);
        sell_it++;
    }
    // 遍历卖方数量时间链，计算买方剩余部分撮合金额
    iBuyMatchQty = value.instCtrl.buyQty - iSellMatchQty;
    auto qty_iter = value.sellerSeries.qtyTimeSeriesList.begin();
    auto qty_time_iter = qty_iter->second.begin();
    while (1) {
        // 买方剩余为0
        if (iBuyMatchQty == 0) {
            break;
        }
        // 该数量所有订单撮合完，到下一个数量
        if (qty_time_iter == qty_iter->second.end()) {
            qty_iter++;
            if (qty_iter == value.sellerSeries.qtyTimeSeriesList.end())
                break;
            else {
                qty_time_iter = qty_iter->second.begin();
                continue;
            }
        }
        
        iSellOrdNo = *qty_time_iter;
        if (m_ord_info_map[iSellOrdNo].matchStatus != MatchStatus::kInit &&
            m_ord_info_map[iSellOrdNo].matchStatus != MatchStatus::kAllDeal) {
            qty_time_iter++;
            continue;
        }
        CalSellMatchQtyByQtyTime(iSellOrdNo, &iBuyMatchQty);
        qty_time_iter++;
    }
}

void ZRTMatcher::CalSellMatchQtyByProportion(const OrderID sell_ord_id,
                                             const double iRate,
                                             Qty* iSellQty) {
    int64_t iBalance = m_ord_info_map[sell_ord_id].leftover * iRate;
    if (iBalance % ZRT_QTY_BASE_VAL) {
        iBalance -= iBalance % ZRT_QTY_BASE_VAL;
    }
    m_ord_info_map[sell_ord_id].matchQty = iBalance;
    *iSellQty += iBalance;
}

void ZRTMatcher::CalSellMatchQtyByQtyTime(const OrderID sell_ord_id,
                                          Qty* iBuyQty) {
    Qty sell_left = m_ord_info_map[sell_ord_id].leftover - m_ord_info_map[sell_ord_id].matchQty;
    // 卖方把剩余全撮合掉
    if (sell_left > *iBuyQty) {
        m_ord_info_map[sell_ord_id].matchQty += *iBuyQty;
        *iBuyQty = 0;
    }
    else {
        m_ord_info_map[sell_ord_id].matchQty += sell_left;
        *iBuyQty -= sell_left;
    }
}

void ZRTMatcher::CheckSetMatchStatus(const OrderID ord_id) {
    m_ord_info_map[ord_id].matchStatus = m_ord_info_map[ord_id].leftover > 0
                                         ?
                                         MatchStatus::kPartial
                                         :
                                         MatchStatus::kAllDeal;
}
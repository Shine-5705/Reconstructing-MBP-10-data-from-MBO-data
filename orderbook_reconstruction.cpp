#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>

struct Order {
    std::string ts_recv;
    std::string ts_event;
    int rtype;
    int publisher_id;
    int instrument_id;
    char action;
    char side;
    double price;
    int size;
    int channel_id;
    long long order_id;
    int flags;
    int ts_in_delta;
    long long sequence;
    std::string symbol;
};

struct PriceLevel {
    double price;
    int size;
    int count;
    
    PriceLevel() : price(0.0), size(0), count(0) {}
    PriceLevel(double p, int s, int c) : price(p), size(s), count(c) {}
};

class OrderBook {
private:
    // Using maps for automatic sorting - bids descending, asks ascending
    std::map<double, std::pair<int, int>, std::greater<double>> bids; // price -> (size, count)
    std::map<double, std::pair<int, int>> asks; // price -> (size, count)
    std::unordered_map<long long, std::pair<double, char>> orders; // order_id -> (price, side)
    
public:
    void clear() {
        bids.clear();
        asks.clear();
        orders.clear();
    }
    
    void addOrder(long long order_id, char side, double price, int size) {
        if (order_id > 0) {
            orders[order_id] = {price, side};
        }
        
        if (side == 'B') {
            bids[price].first += size;
            bids[price].second += 1;
        } else if (side == 'A') {
            asks[price].first += size;
            asks[price].second += 1;
        }
    }
    
    void cancelOrder(long long order_id, char side, double price, int size) {
        if (side == 'B') {
            auto it = bids.find(price);
            if (it != bids.end()) {
                it->second.first -= size;
                it->second.second -= 1;
                if (it->second.first <= 0 || it->second.second <= 0) {
                    bids.erase(it);
                }
            }
        } else if (side == 'A') {
            auto it = asks.find(price);
            if (it != asks.end()) {
                it->second.first -= size;
                it->second.second -= 1;
                if (it->second.first <= 0 || it->second.second <= 0) {
                    asks.erase(it);
                }
            }
        }
        if (order_id > 0) {
            orders.erase(order_id);
        }
    }
    
    void executeTrade(char trade_side, double trade_price, int trade_size) {
        // Trade removes liquidity from the opposite side of the book
        char affected_side = (trade_side == 'A') ? 'B' : 'A';
        
        if (affected_side == 'B') {
            auto it = bids.find(trade_price);
            if (it != bids.end()) {
                it->second.first -= trade_size;
                if (it->second.first <= 0) {
                    bids.erase(it);
                }
            }
        } else {
            auto it = asks.find(trade_price);
            if (it != asks.end()) {
                it->second.first -= trade_size;
                if (it->second.first <= 0) {
                    asks.erase(it);
                }
            }
        }
    }
    
    std::vector<PriceLevel> getBids(int depth = 10) const {
        std::vector<PriceLevel> result;
        int count = 0;
        for (const auto& bid : bids) {
            if (count >= depth) break;
            result.emplace_back(bid.first, bid.second.first, bid.second.second);
            count++;
        }
        return result;
    }
    
    std::vector<PriceLevel> getAsks(int depth = 10) const {
        std::vector<PriceLevel> result;
        int count = 0;
        for (const auto& ask : asks) {
            if (count >= depth) break;
            result.emplace_back(ask.first, ask.second.first, ask.second.second);
            count++;
        }
        return result;
    }
    
    int findDepth(char side, double price) const {
        if (side == 'B') {
            int depth = 0;
            for (const auto& bid : bids) {
                if (std::fabs(bid.first - price) < 0.001) {
                    return depth;
                }
                depth++;
            }
        } else if (side == 'A') {
            int depth = 0;
            for (const auto& ask : asks) {
                if (std::fabs(ask.first - price) < 0.001) {
                    return depth;
                }
                depth++;
            }
        }
        return 0;
    }
};

Order parseCSVLine(const std::string& line) {
    Order order;
    std::stringstream ss(line);
    std::string field;
    int field_num = 0;
    
    while (std::getline(ss, field, ',')) {
        switch(field_num) {
            case 0: order.ts_recv = field; break;
            case 1: order.ts_event = field; break;
            case 2: order.rtype = field.empty() ? 0 : std::stoi(field); break;
            case 3: order.publisher_id = field.empty() ? 0 : std::stoi(field); break;
            case 4: order.instrument_id = field.empty() ? 0 : std::stoi(field); break;
            case 5: order.action = field.empty() ? 'X' : field[0]; break;
            case 6: order.side = field.empty() ? 'N' : field[0]; break;
            case 7: order.price = field.empty() ? 0.0 : std::stod(field); break;
            case 8: order.size = field.empty() ? 0 : std::stoi(field); break;
            case 9: order.channel_id = field.empty() ? 0 : std::stoi(field); break;
            case 10: order.order_id = field.empty() ? 0 : std::stoll(field); break;
            case 11: order.flags = field.empty() ? 0 : std::stoi(field); break;
            case 12: order.ts_in_delta = field.empty() ? 0 : std::stoi(field); break;
            case 13: order.sequence = field.empty() ? 0 : std::stoll(field); break;
            case 14: order.symbol = field; break;
        }
        field_num++;
    }
    return order;
}

void writeOutputRow(std::ofstream& out, const Order& order, const OrderBook& book, int row_num) {
    auto bids = book.getBids(10);
    auto asks = book.getAsks(10);
    
    // Determine the correct depth value
    int depth = book.findDepth(order.side, order.price);
    
    out << row_num << ","
        << order.ts_event << "," // Use ts_event for both fields to match expected output
        << order.ts_event << ","
        << "10," // rtype for MBP
        << order.publisher_id << ","
        << order.instrument_id << ","
        << order.action << ","
        << order.side << ","
        << depth << ",";
    
    // Handle price formatting - empty if 0
    if (order.price == 0.0) {
        out << ",";
    } else {
        out << order.price << ",";
    }
    
    out << order.size << ","
        << order.flags << ","
        << order.ts_in_delta << ","
        << order.sequence;
    
    // Output interleaved bid/ask levels (10 levels each)
    for (int i = 0; i < 10; i++) {
        // Bid level
        if (i < static_cast<int>(bids.size())) {
            out << "," << bids[i].price
                << "," << bids[i].size
                << "," << bids[i].count;
        } else {
            out << ",,0,0";
        }
        
        // Ask level
        if (i < static_cast<int>(asks.size())) {
            out << "," << asks[i].price
                << "," << asks[i].size
                << "," << asks[i].count;
        } else {
            out << ",,0,0";
        }
    }
    
    out << "," << order.symbol << "," << order.order_id << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <mbo_file.csv>" << std::endl;
        return 1;
    }
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::ifstream input(argv[1]);
    if (!input.is_open()) {
        std::cerr << "Error: Could not open file " << argv[1] << std::endl;
        return 1;
    }
    
    std::ofstream output("output_mbp.csv");
    if (!output.is_open()) {
        std::cerr << "Error: Could not create output file" << std::endl;
        return 1;
    }
    
    // Write header
    output << ",ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,depth,price,size,flags,ts_in_delta,sequence";
    for (int i = 0; i < 10; i++) {
        output << ",bid_px_" << std::setfill('0') << std::setw(2) << i
               << ",bid_sz_" << std::setfill('0') << std::setw(2) << i
               << ",bid_ct_" << std::setfill('0') << std::setw(2) << i
               << ",ask_px_" << std::setfill('0') << std::setw(2) << i
               << ",ask_sz_" << std::setfill('0') << std::setw(2) << i
               << ",ask_ct_" << std::setfill('0') << std::setw(2) << i;
    }
    output << ",symbol,order_id" << std::endl;
    
    OrderBook book;
    std::string line;
    int row_num = 0;
    bool first_line = true;
    
    // Skip header line
    std::getline(input, line);
    
    std::vector<Order> trade_sequence;
    
    while (std::getline(input, line)) {
        if (line.empty()) continue;
        
        Order order = parseCSVLine(line);
        
        // Skip first row as it's a clear action
        if (first_line && order.action == 'R') {
            book.clear();
            writeOutputRow(output, order, book, row_num++);
            first_line = false;
            continue;
        }
        first_line = false;
        
        // Handle T->F->C sequence
        if (order.action == 'T') {
            if (order.side == 'N') {
                // Skip trades with side N
                continue;
            }
            trade_sequence.clear();
            trade_sequence.push_back(order);
            continue;
        } else if (order.action == 'F') {
            trade_sequence.push_back(order);
            continue;
        } else if (order.action == 'C' && !trade_sequence.empty()) {
            // Complete T->F->C sequence
            Order trade_order = trade_sequence[0];
            
            // Execute the trade (affects opposite side)
            book.executeTrade(trade_order.side, trade_order.price, trade_order.size);
            
            // Output the trade with the affected side
            Order output_order = trade_order;
            output_order.side = (trade_order.side == 'B') ? 'A' : 'B';
            writeOutputRow(output, output_order, book, row_num++);
            
            // Then process the cancel
            book.cancelOrder(order.order_id, order.side, order.price, order.size);
            writeOutputRow(output, order, book, row_num++);
            
            trade_sequence.clear();
            continue;
        }
        
        // Handle regular actions
        if (order.action == 'A') {
            // Add order
            book.addOrder(order.order_id, order.side, order.price, order.size);
            writeOutputRow(output, order, book, row_num++);
            
        } else if (order.action == 'C') {
            // Cancel order
            book.cancelOrder(order.order_id, order.side, order.price, order.size);
            writeOutputRow(output, order, book, row_num++);
            
        } else {
            // Other actions, just output current state
            writeOutputRow(output, order, book, row_num++);
        }
    }
    
    input.close();
    output.close();
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "Orderbook reconstruction completed successfully!" << std::endl;
    std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
    std::cout << "Output written to: output_mbp.csv" << std::endl;
    
    return 0;
}

#include <iostream>
#include <fstream>
#include <string>
#include <cassert>
#include <vector>
#include <chrono>

// Simple test framework
void testOrderBookBasics() {
    std::cout << "Testing basic orderbook operations..." << std::endl;
    
    // Create a simple test CSV
    std::ofstream test_input("test_input.csv");
    test_input << "ts_recv,ts_event,rtype,publisher_id,instrument_id,action,side,price,size,channel_id,order_id,flags,ts_in_delta,sequence,symbol\n";
    test_input << "2025-07-17T07:05:09.035793433Z,2025-07-17T07:05:09.035627674Z,160,2,1108,R,N,,0,0,0,8,0,0,ARL\n";
    test_input << "2025-07-17T08:05:03.360842448Z,2025-07-17T08:05:03.360677248Z,160,2,1108,A,B,5.510000000,100,0,817593,130,165200,851012,ARL\n";
    test_input << "2025-07-17T08:05:03.360848793Z,2025-07-17T08:05:03.360683462Z,160,2,1108,A,A,21.330000000,100,0,817597,130,165331,851013,ARL\n";
    test_input.close();
    
    // Run the program
    int result = system(".\\reconstruction_gupta.exe test_input.csv");
    assert(result == 0);
    
    // Check output file exists
    std::ifstream output("output_mbp.csv");
    assert(output.is_open());
    
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(output, line)) {
        lines.push_back(line);
    }
    output.close();
    
    // Basic checks
    assert(lines.size() > 3); // Header + at least 3 data rows
    assert(lines[0].find("ts_recv") != std::string::npos); // Header exists
    assert(lines[1].find("R,N") != std::string::npos); // Clear action
    assert(lines[2].find("A,B") != std::string::npos); // Add bid
    assert(lines[3].find("A,A") != std::string::npos); // Add ask
    
    std::cout << "Basic tests passed!" << std::endl;
    
    // Cleanup
    remove("test_input.csv");
    remove("output_mbp.csv");
}

void testPerformance() {
    std::cout << "Testing performance with main dataset..." << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    int result = system(".\\reconstruction_gupta.exe mbo.csv");
    auto end = std::chrono::high_resolution_clock::now();
    
    assert(result == 0);
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Processing time: " << duration.count() << "ms" << std::endl;
    
    // Check that it completes in reasonable time (< 5 seconds)
    assert(duration.count() < 5000);
    
    std::cout << "Performance test passed!" << std::endl;
}

int main() {
    try {
        testOrderBookBasics();
        testPerformance();
        std::cout << "All tests passed!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown error" << std::endl;
        return 1;
    }
    
    return 0;
}

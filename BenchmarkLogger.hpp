//
// Created by cds on 2020/9/18.
//

#include <fstream>
#include <vector>
#include <tuple>
#include <thread>

using namespace std;

class BenchmarkLogger
{
public:
    static std::vector<std::tuple<std::string, uint64_t, uint32_t>> latencyResults; // src + seq# + latency
    static void DumpResultsToFile()
    {
        stringstream ss;
        ss << std::this_thread::get_id();
        string PID = ss.str();
        std::ofstream outfile("BenchmarkResults_" + PID + ".csv");
        outfile << "src,seq#,latency (microsec)" << endl;
        for (auto &result : latencyResults)
        {
            outfile << std::get<0>(result) << "," << std::get<1>(result) << "," << std::get<2>(result) << endl;
        }
        outfile.clear();
    };
};

std::vector<std::tuple<std::string, uint64_t, uint32_t>> BenchmarkLogger::latencyResults;


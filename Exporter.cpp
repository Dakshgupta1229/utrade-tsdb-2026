#include "Exporter.h"
#include <fstream>
#include <iomanip>

bool exportToCSV(const TimeSeries& ts,
                 const std::string& metric,
                 long long t_start,
                 long long t_end,
                 const std::string& filename) {
    auto points = ts.query(metric, t_start, t_end);

    std::ofstream file(filename);
    if (!file.is_open()) return false;

    // Header row
    file << "timestamp,value\n";

    // Write each data point with full double precision
    file << std::fixed << std::setprecision(6);
    for (const auto& p : points) {
        file << p.timestamp << "," << p.value << "\n";
    }

    return file.good();
}

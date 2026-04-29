#pragma once

#include "TimeSeries.h"
#include <string>

// Exports a time range of a metric to a CSV file
// Format: timestamp,value (one row per data point, with header)
// Returns true on success, false on file I/O error
bool exportToCSV(const TimeSeries& ts,
                 const std::string& metric,
                 long long t_start,
                 long long t_end,
                 const std::string& filename);

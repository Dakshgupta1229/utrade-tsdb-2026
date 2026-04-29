#pragma once

#include <map>
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>

// Represents a single data point: timestamp + value
struct DataPoint {
    long long timestamp;
    double value;
};

// Aggregation types supported by the engine
enum class AggType { MIN, MAX, AVG, COUNT };

// Core time-series storage engine
// Uses: map<metric_name, map<timestamp, value>>
// - Outer map: O(log M) lookup by metric name
// - Inner map: O(log N) insert + O(log N + K) range query via lower/upper_bound
class TimeSeries {
public:
    // Insert a value for a metric at a given timestamp
    // Overwrites if (metric, timestamp) already exists
    void insert(const std::string& metric, long long timestamp, double value);

    // Query all data points in [t_start, t_end] (inclusive)
    std::vector<DataPoint> query(const std::string& metric, long long t_start, long long t_end) const;

    // Aggregate (MIN/MAX/AVG/COUNT) over [t_start, t_end]
    std::optional<double> aggregate(const std::string& metric, long long t_start, long long t_end, AggType type) const;

    // Get the latest N data points for a metric
    std::vector<DataPoint> latest(const std::string& metric, size_t n) const;

    // Delete all data for a metric
    bool deleteMetric(const std::string& metric);

    // Delete all data points in [t_start, t_end] for a metric
    size_t deleteRange(const std::string& metric, long long t_start, long long t_end);

    // Apply retention policy: remove all points older than (now - seconds)
    size_t applyRetention(const std::string& metric, long long now, long long retentionSeconds);

    // List all metric names
    std::vector<std::string> listMetrics() const;

    // Check if a metric exists
    bool hasMetric(const std::string& metric) const;

    // Get total data point count for a metric
    size_t count(const std::string& metric) const;

private:
    // metric_name -> (timestamp -> value)
    std::map<std::string, std::map<long long, double>> data_;
};

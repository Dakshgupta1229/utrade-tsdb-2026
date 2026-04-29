#include "TimeSeries.h"
#include <algorithm>
#include <numeric>
#include <limits>
#include <stdexcept>

void TimeSeries::insert(const std::string& metric, long long timestamp, double value) {
    if (metric.empty()) throw std::invalid_argument("Metric name cannot be empty");
    data_[metric][timestamp] = value;
}

std::vector<DataPoint> TimeSeries::query(const std::string& metric, long long t_start, long long t_end) const {
    if (t_start > t_end) throw std::invalid_argument("t_start must be <= t_end");

    auto it = data_.find(metric);
    if (it == data_.end()) return {};

    const auto& series = it->second;
    // lower_bound gives first key >= t_start
    // upper_bound gives first key > t_end
    auto begin = series.lower_bound(t_start);
    auto end   = series.upper_bound(t_end);

    std::vector<DataPoint> result;
    for (auto cur = begin; cur != end; ++cur) {
        result.push_back({cur->first, cur->second});
    }
    return result;
}

std::optional<double> TimeSeries::aggregate(const std::string& metric, long long t_start, long long t_end, AggType type) const {
    auto points = query(metric, t_start, t_end);
    if (points.empty()) return std::nullopt;

    switch (type) {
        case AggType::COUNT:
            return static_cast<double>(points.size());

        case AggType::MIN: {
            double mn = points[0].value;
            for (const auto& p : points) mn = std::min(mn, p.value);
            return mn;
        }

        case AggType::MAX: {
            double mx = points[0].value;
            for (const auto& p : points) mx = std::max(mx, p.value);
            return mx;
        }

        case AggType::AVG: {
            double sum = 0.0;
            for (const auto& p : points) sum += p.value;
            return sum / static_cast<double>(points.size());
        }
    }
    return std::nullopt; // unreachable
}

std::vector<DataPoint> TimeSeries::latest(const std::string& metric, size_t n) const {
    auto it = data_.find(metric);
    if (it == data_.end()) return {};

    const auto& series = it->second;
    std::vector<DataPoint> result;

    // Walk backwards from the end of the sorted map
    auto cur = series.end();
    size_t count = 0;
    while (cur != series.begin() && count < n) {
        --cur;
        result.push_back({cur->first, cur->second});
        ++count;
    }

    // Reverse so results are in chronological order
    std::reverse(result.begin(), result.end());
    return result;
}

bool TimeSeries::deleteMetric(const std::string& metric) {
    return data_.erase(metric) > 0;
}

size_t TimeSeries::deleteRange(const std::string& metric, long long t_start, long long t_end) {
    auto it = data_.find(metric);
    if (it == data_.end()) return 0;

    auto& series = it->second;
    auto begin = series.lower_bound(t_start);
    auto end   = series.upper_bound(t_end);

    size_t removed = std::distance(begin, end);
    series.erase(begin, end);

    // Clean up empty metric
    if (series.empty()) data_.erase(it);
    return removed;
}

size_t TimeSeries::applyRetention(const std::string& metric, long long now, long long retentionSeconds) {
    long long cutoff = now - retentionSeconds;
    return deleteRange(metric, std::numeric_limits<long long>::min(), cutoff - 1);
}

std::vector<std::string> TimeSeries::listMetrics() const {
    std::vector<std::string> names;
    names.reserve(data_.size());
    for (const auto& [name, _] : data_) {
        names.push_back(name);
    }
    return names;
}

bool TimeSeries::hasMetric(const std::string& metric) const {
    return data_.count(metric) > 0;
}

size_t TimeSeries::count(const std::string& metric) const {
    auto it = data_.find(metric);
    if (it == data_.end()) return 0;
    return it->second.size();
}

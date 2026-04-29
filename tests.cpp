// Minimal self-contained unit tests (no external framework needed)
// Build with: g++ -std=c++17 -I../include tests.cpp ../src/TimeSeries.cpp -o tests && ./tests

#include "TimeSeries.h"
#include "Parser.h"
#include <cassert>
#include <iostream>
#include <cmath>

// ─── helpers ────────────────────────────────────────────────────────────────

static int passed = 0, failed = 0;

#define TEST(name) \
    std::cout << "  TEST " << #name << " ... "; \
    try {

#define END_TEST \
        std::cout << "PASS\n"; ++passed; \
    } catch (const std::exception& e) { \
        std::cout << "FAIL: " << e.what() << "\n"; ++failed; \
    }

#define ASSERT(cond) \
    if (!(cond)) throw std::runtime_error("Assertion failed: " #cond)

#define ASSERT_NEAR(a, b, eps) \
    if (std::abs((a)-(b)) > (eps)) \
        throw std::runtime_error("Values not close enough: " + std::to_string(a) + " vs " + std::to_string(b))

// ─── TimeSeries tests ────────────────────────────────────────────────────────

static void testInsertAndQuery() {
    std::cout << "\n[TimeSeries]\n";

    TEST(basic_insert_query)
        TimeSeries ts;
        ts.insert("cpu", 1000, 72.5);
        ts.insert("cpu", 1060, 85.3);
        ts.insert("cpu", 1120, 90.1);

        auto pts = ts.query("cpu", 1000, 1120);
        ASSERT(pts.size() == 3);
        ASSERT_NEAR(pts[0].value, 72.5, 1e-9);
        ASSERT_NEAR(pts[2].value, 90.1, 1e-9);
    END_TEST

    TEST(query_subrange)
        TimeSeries ts;
        ts.insert("cpu", 1000, 10.0);
        ts.insert("cpu", 2000, 20.0);
        ts.insert("cpu", 3000, 30.0);

        auto pts = ts.query("cpu", 1500, 2500);
        ASSERT(pts.size() == 1);
        ASSERT(pts[0].timestamp == 2000);
    END_TEST

    TEST(query_empty_metric)
        TimeSeries ts;
        auto pts = ts.query("nonexistent", 0, 9999);
        ASSERT(pts.empty());
    END_TEST

    TEST(overwrite_same_timestamp)
        TimeSeries ts;
        ts.insert("mem", 1000, 50.0);
        ts.insert("mem", 1000, 99.0);  // overwrite
        auto pts = ts.query("mem", 1000, 1000);
        ASSERT(pts.size() == 1);
        ASSERT_NEAR(pts[0].value, 99.0, 1e-9);
    END_TEST

    TEST(multi_metric_isolation)
        TimeSeries ts;
        ts.insert("cpu", 1000, 80.0);
        ts.insert("ram", 1000, 40.0);

        auto cpu = ts.query("cpu", 0, 9999);
        auto ram = ts.query("ram", 0, 9999);
        ASSERT(cpu.size() == 1 && ram.size() == 1);
        ASSERT_NEAR(cpu[0].value, 80.0, 1e-9);
        ASSERT_NEAR(ram[0].value, 40.0, 1e-9);
    END_TEST
}

static void testAggregation() {
    std::cout << "\n[Aggregation]\n";

    TEST(agg_min_max_avg_count)
        TimeSeries ts;
        ts.insert("x", 1, 10.0);
        ts.insert("x", 2, 20.0);
        ts.insert("x", 3, 30.0);

        ASSERT_NEAR(ts.aggregate("x", 1, 3, AggType::MIN).value(),   10.0, 1e-9);
        ASSERT_NEAR(ts.aggregate("x", 1, 3, AggType::MAX).value(),   30.0, 1e-9);
        ASSERT_NEAR(ts.aggregate("x", 1, 3, AggType::AVG).value(),   20.0, 1e-9);
        ASSERT_NEAR(ts.aggregate("x", 1, 3, AggType::COUNT).value(), 3.0,  1e-9);
    END_TEST

    TEST(agg_empty_range)
        TimeSeries ts;
        ts.insert("x", 100, 5.0);
        auto result = ts.aggregate("x", 200, 300, AggType::AVG);
        ASSERT(!result.has_value());
    END_TEST
}

static void testLatest() {
    std::cout << "\n[Latest]\n";

    TEST(latest_returns_chronological)
        TimeSeries ts;
        ts.insert("s", 1, 1.0);
        ts.insert("s", 2, 2.0);
        ts.insert("s", 3, 3.0);

        auto pts = ts.latest("s", 2);
        ASSERT(pts.size() == 2);
        ASSERT(pts[0].timestamp == 2);
        ASSERT(pts[1].timestamp == 3);
    END_TEST

    TEST(latest_n_greater_than_size)
        TimeSeries ts;
        ts.insert("s", 1, 1.0);
        ts.insert("s", 2, 2.0);

        auto pts = ts.latest("s", 10);
        ASSERT(pts.size() == 2);
    END_TEST
}

static void testDelete() {
    std::cout << "\n[Delete]\n";

    TEST(delete_metric)
        TimeSeries ts;
        ts.insert("del", 1, 1.0);
        ASSERT(ts.hasMetric("del"));
        ASSERT(ts.deleteMetric("del"));
        ASSERT(!ts.hasMetric("del"));
    END_TEST

    TEST(delete_range)
        TimeSeries ts;
        ts.insert("r", 1, 1.0);
        ts.insert("r", 2, 2.0);
        ts.insert("r", 3, 3.0);

        size_t removed = ts.deleteRange("r", 1, 2);
        ASSERT(removed == 2);
        ASSERT(ts.count("r") == 1);
    END_TEST
}

static void testParser() {
    std::cout << "\n[Parser]\n";

    TEST(parse_insert)
        auto cmd = parseLine("INSERT cpu 1000 72.5");
        ASSERT(cmd.type == CommandType::INSERT);
        ASSERT(cmd.metric == "cpu");
        ASSERT(cmd.t_start == 1000);
        ASSERT_NEAR(cmd.value, 72.5, 1e-9);
    END_TEST

    TEST(parse_query)
        auto cmd = parseLine("QUERY cpu 1000 2000");
        ASSERT(cmd.type == CommandType::QUERY);
        ASSERT(cmd.t_start == 1000 && cmd.t_end == 2000);
    END_TEST

    TEST(parse_agg)
        auto cmd = parseLine("AGG cpu 0 9999 MAX");
        ASSERT(cmd.type == CommandType::AGG);
        ASSERT(cmd.agg_type == "MAX");
    END_TEST

    TEST(parse_case_insensitive)
        auto cmd = parseLine("insert cpu 1000 55.0");
        ASSERT(cmd.type == CommandType::INSERT);
    END_TEST

    TEST(parse_invalid_agg_type)
        auto cmd = parseLine("AGG cpu 0 9999 MEDIAN");
        ASSERT(cmd.type == CommandType::UNKNOWN);
    END_TEST

    TEST(parse_unknown_command)
        auto cmd = parseLine("FOOBAR cpu");
        ASSERT(cmd.type == CommandType::UNKNOWN);
    END_TEST

    TEST(parse_empty_line)
        auto cmd = parseLine("   ");
        ASSERT(cmd.type == CommandType::UNKNOWN);
    END_TEST

    TEST(parse_latest)
        auto cmd = parseLine("LATEST cpu 5");
        ASSERT(cmd.type == CommandType::LATEST);
        ASSERT(cmd.metric == "cpu");
        ASSERT(cmd.n == 5);
    END_TEST

    TEST(parse_delete)
        auto cmd = parseLine("DELETE cpu");
        ASSERT(cmd.type == CommandType::DELETE);
    END_TEST

    TEST(parse_delete_range)
        auto cmd = parseLine("DELETE cpu 1000 2000");
        ASSERT(cmd.type == CommandType::DELETE_RANGE);
        ASSERT(cmd.t_start == 1000 && cmd.t_end == 2000);
    END_TEST

    TEST(parse_export)
        auto cmd = parseLine("EXPORT cpu 1000 2000 out.csv");
        ASSERT(cmd.type == CommandType::EXPORT);
        ASSERT(cmd.filename == "out.csv");
    END_TEST
}

// ─── entry point ────────────────────────────────────────────────────────────

int main() {
    std::cout << "Running TSDB tests...\n";

    testInsertAndQuery();
    testAggregation();
    testLatest();
    testDelete();
    testParser();

    std::cout << "\n────────────────────────────\n";
    std::cout << "Results: " << passed << " passed, " << failed << " failed\n";
    return failed == 0 ? 0 : 1;
}

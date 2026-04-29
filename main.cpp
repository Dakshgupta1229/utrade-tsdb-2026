#include "TimeSeries.h"
#include "Parser.h"
#include "Exporter.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <chrono>
#include <limits>

// Returns current Unix timestamp in seconds
static long long nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

// Map AggType string to enum
static AggType toAggType(const std::string& s) {
    if (s == "MIN")   return AggType::MIN;
    if (s == "MAX")   return AggType::MAX;
    if (s == "AVG")   return AggType::AVG;
    if (s == "COUNT") return AggType::COUNT;
    throw std::invalid_argument("Unknown agg: " + s);
}

static void printHelp() {
    std::cout <<
        "\n╔══════════════════════════════════════════════════════════════╗\n"
        "║            TSDB — In-Memory Time-Series Database            ║\n"
        "╠══════════════════════════════════════════════════════════════╣\n"
        "║  INSERT  <metric> <timestamp> <value>                       ║\n"
        "║  QUERY   <metric> <t_start> <t_end>                         ║\n"
        "║  AGG     <metric> <t_start> <t_end> <MIN|MAX|AVG|COUNT>     ║\n"
        "║  LATEST  <metric> <n>                                       ║\n"
        "║  DELETE  <metric>                                           ║\n"
        "║  DELETE  <metric> <t_start> <t_end>      (range delete)     ║\n"
        "║  EXPORT  <metric> <t_start> <t_end> <file.csv>              ║\n"
        "║  RETENTION <metric> <seconds>            (prune old data)   ║\n"
        "║  LIST                                    (all metrics)      ║\n"
        "║  COUNT   <metric>                        (data point count) ║\n"
        "║  HELP                                                       ║\n"
        "║  EXIT                                                       ║\n"
        "╠══════════════════════════════════════════════════════════════╣\n"
        "║  Timestamps: Unix epoch seconds (e.g. 1714900000)           ║\n"
        "║  Use 'now' keyword in place of any timestamp                ║\n"
        "╚══════════════════════════════════════════════════════════════╝\n\n";
}

// Replace the literal word "now" in a token with the current epoch second
static std::string resolveNow(const std::string& tok) {
    if (tok == "now") return std::to_string(nowSeconds());
    return tok;
}

// Pre-process tokens: replace "now" before parsing
static std::vector<std::string> resolveTokens(std::vector<std::string> tokens) {
    for (auto& t : tokens) t = resolveNow(t);
    return tokens;
}

static void printPoints(const std::vector<DataPoint>& pts) {
    if (pts.empty()) {
        std::cout << "(no data)\n";
        return;
    }
    std::cout << std::fixed << std::setprecision(4);
    for (const auto& p : pts) {
        std::cout << "  " << p.timestamp << "  \t" << p.value << "\n";
    }
    std::cout << "(" << pts.size() << " point" << (pts.size() != 1 ? "s" : "") << ")\n";
}

int main() {
    TimeSeries db;
    std::string line;

    printHelp();
    std::cout << "TSDB ready. Type HELP for commands.\n\n";

    while (true) {
        std::cout << "tsdb> ";
        if (!std::getline(std::cin, line)) break;  // EOF (Ctrl+D)

        // Skip blank lines and comments
        if (line.empty() || line[0] == '#') continue;

        auto tokens  = resolveTokens(tokenize(line));
        Command cmd  = parseCommand(tokens);

        try {
            switch (cmd.type) {

                case CommandType::INSERT:
                    db.insert(cmd.metric, cmd.t_start, cmd.value);
                    std::cout << "OK\n";
                    break;

                case CommandType::QUERY: {
                    auto pts = db.query(cmd.metric, cmd.t_start, cmd.t_end);
                    printPoints(pts);
                    break;
                }

                case CommandType::AGG: {
                    auto result = db.aggregate(cmd.metric, cmd.t_start, cmd.t_end, toAggType(cmd.agg_type));
                    if (!result.has_value()) {
                        std::cout << "(no data in range)\n";
                    } else {
                        std::cout << std::fixed << std::setprecision(4) << result.value() << "\n";
                    }
                    break;
                }

                case CommandType::LATEST: {
                    auto pts = db.latest(cmd.metric, static_cast<size_t>(cmd.n));
                    printPoints(pts);
                    break;
                }

                case CommandType::DELETE:
                    if (db.deleteMetric(cmd.metric)) {
                        std::cout << "Deleted metric '" << cmd.metric << "'\n";
                    } else {
                        std::cout << "Metric '" << cmd.metric << "' not found\n";
                    }
                    break;

                case CommandType::DELETE_RANGE: {
                    size_t n = db.deleteRange(cmd.metric, cmd.t_start, cmd.t_end);
                    std::cout << "Deleted " << n << " point(s)\n";
                    break;
                }

                case CommandType::EXPORT: {
                    bool ok = exportToCSV(db, cmd.metric, cmd.t_start, cmd.t_end, cmd.filename);
                    if (ok) {
                        std::cout << "Exported to " << cmd.filename << "\n";
                    } else {
                        std::cerr << "Error: could not write to " << cmd.filename << "\n";
                    }
                    break;
                }

                case CommandType::RETENTION: {
                    size_t n = db.applyRetention(cmd.metric, nowSeconds(), cmd.retention_secs);
                    std::cout << "Pruned " << n << " old point(s)\n";
                    break;
                }

                case CommandType::LIST: {
                    auto metrics = db.listMetrics();
                    if (metrics.empty()) {
                        std::cout << "(no metrics stored)\n";
                    } else {
                        for (const auto& m : metrics) {
                            std::cout << "  " << m << "  (" << db.count(m) << " points)\n";
                        }
                    }
                    break;
                }

                case CommandType::COUNT:
                    std::cout << db.count(cmd.metric) << " point(s)\n";
                    break;

                case CommandType::HELP:
                    printHelp();
                    break;

                case CommandType::EXIT:
                    std::cout << "Goodbye.\n";
                    return 0;

                case CommandType::UNKNOWN:
                default:
                    std::cerr << "Error: " << cmd.raw << "\n";
                    break;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << "\n";
        }
    }

    return 0;
}

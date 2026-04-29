#pragma once

#include <string>
#include <vector>
#include <optional>

// Supported command types
enum class CommandType {
    INSERT,
    QUERY,
    AGG,
    LATEST,
    DELETE,
    DELETE_RANGE,
    EXPORT,
    RETENTION,
    LIST,
    COUNT,
    HELP,
    EXIT,
    UNKNOWN
};

// Parsed representation of a user command
struct Command {
    CommandType type = CommandType::UNKNOWN;
    std::string metric;
    long long   t_start  = 0;
    long long   t_end    = 0;
    double      value    = 0.0;
    int         n        = 0;          // for LATEST
    std::string agg_type;              // "MIN", "MAX", "AVG", "COUNT"
    std::string filename;              // for EXPORT
    long long   retention_secs = 0;   // for RETENTION
    std::string raw;                   // original input line (for error messages)
};

// Splits a string by whitespace into tokens
std::vector<std::string> tokenize(const std::string& line);

// Parses a tokenized line into a Command struct
// Returns Command with type=UNKNOWN and an error message in `raw` on failure
Command parseCommand(const std::vector<std::string>& tokens);

// Convenience wrapper: tokenize then parse
Command parseLine(const std::string& line);

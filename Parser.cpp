#include "Parser.h"
#include <sstream>
#include <algorithm>
#include <stdexcept>

std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string tok;
    while (iss >> tok) tokens.push_back(tok);
    return tokens;
}

// Convert string to uppercase for case-insensitive command matching
static std::string toUpper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

// Safely parse a long long; throws std::invalid_argument on failure
static long long parseLong(const std::string& s) {
    size_t pos;
    long long val = std::stoll(s, &pos);
    if (pos != s.size()) throw std::invalid_argument("Not a valid integer: " + s);
    return val;
}

// Safely parse a double; throws std::invalid_argument on failure
static double parseDouble(const std::string& s) {
    size_t pos;
    double val = std::stod(s, &pos);
    if (pos != s.size()) throw std::invalid_argument("Not a valid number: " + s);
    return val;
}

Command parseCommand(const std::vector<std::string>& tokens) {
    Command cmd;
    if (tokens.empty()) {
        cmd.type = CommandType::UNKNOWN;
        return cmd;
    }

    std::string verb = toUpper(tokens[0]);

    try {
        // INSERT <metric> <timestamp> <value>
        if (verb == "INSERT") {
            if (tokens.size() != 4) {
                cmd.raw = "Usage: INSERT <metric> <timestamp> <value>";
                return cmd;
            }
            cmd.type      = CommandType::INSERT;
            cmd.metric    = tokens[1];
            cmd.t_start   = parseLong(tokens[2]);
            cmd.value     = parseDouble(tokens[3]);
        }
        // QUERY <metric> <t_start> <t_end>
        else if (verb == "QUERY") {
            if (tokens.size() != 4) {
                cmd.raw = "Usage: QUERY <metric> <t_start> <t_end>";
                return cmd;
            }
            cmd.type    = CommandType::QUERY;
            cmd.metric  = tokens[1];
            cmd.t_start = parseLong(tokens[2]);
            cmd.t_end   = parseLong(tokens[3]);
        }
        // AGG <metric> <t_start> <t_end> <MIN|MAX|AVG|COUNT>
        else if (verb == "AGG") {
            if (tokens.size() != 5) {
                cmd.raw = "Usage: AGG <metric> <t_start> <t_end> <MIN|MAX|AVG|COUNT>";
                return cmd;
            }
            cmd.type     = CommandType::AGG;
            cmd.metric   = tokens[1];
            cmd.t_start  = parseLong(tokens[2]);
            cmd.t_end    = parseLong(tokens[3]);
            cmd.agg_type = toUpper(tokens[4]);
            if (cmd.agg_type != "MIN" && cmd.agg_type != "MAX" &&
                cmd.agg_type != "AVG" && cmd.agg_type != "COUNT") {
                cmd.type = CommandType::UNKNOWN;
                cmd.raw  = "AGG type must be one of: MIN, MAX, AVG, COUNT";
                return cmd;
            }
        }
        // LATEST <metric> <n>
        else if (verb == "LATEST") {
            if (tokens.size() != 3) {
                cmd.raw = "Usage: LATEST <metric> <n>";
                return cmd;
            }
            cmd.type   = CommandType::LATEST;
            cmd.metric = tokens[1];
            cmd.n      = static_cast<int>(parseLong(tokens[2]));
            if (cmd.n <= 0) {
                cmd.type = CommandType::UNKNOWN;
                cmd.raw  = "N must be a positive integer";
                return cmd;
            }
        }
        // DELETE <metric>
        else if (verb == "DELETE") {
            if (tokens.size() == 2) {
                cmd.type   = CommandType::DELETE;
                cmd.metric = tokens[1];
            } else if (tokens.size() == 4) {
                // DELETE <metric> <t_start> <t_end>  — range delete
                cmd.type    = CommandType::DELETE_RANGE;
                cmd.metric  = tokens[1];
                cmd.t_start = parseLong(tokens[2]);
                cmd.t_end   = parseLong(tokens[3]);
            } else {
                cmd.raw = "Usage: DELETE <metric>  OR  DELETE <metric> <t_start> <t_end>";
                return cmd;
            }
        }
        // EXPORT <metric> <t_start> <t_end> <filename>
        else if (verb == "EXPORT") {
            if (tokens.size() != 5) {
                cmd.raw = "Usage: EXPORT <metric> <t_start> <t_end> <filename>";
                return cmd;
            }
            cmd.type     = CommandType::EXPORT;
            cmd.metric   = tokens[1];
            cmd.t_start  = parseLong(tokens[2]);
            cmd.t_end    = parseLong(tokens[3]);
            cmd.filename = tokens[4];
        }
        // RETENTION <metric> <seconds>
        else if (verb == "RETENTION") {
            if (tokens.size() != 3) {
                cmd.raw = "Usage: RETENTION <metric> <seconds>";
                return cmd;
            }
            cmd.type           = CommandType::RETENTION;
            cmd.metric         = tokens[1];
            cmd.retention_secs = parseLong(tokens[2]);
        }
        // LIST
        else if (verb == "LIST") {
            cmd.type = CommandType::LIST;
        }
        // COUNT <metric>
        else if (verb == "COUNT") {
            if (tokens.size() != 2) {
                cmd.raw = "Usage: COUNT <metric>";
                return cmd;
            }
            cmd.type   = CommandType::COUNT;
            cmd.metric = tokens[1];
        }
        // HELP
        else if (verb == "HELP") {
            cmd.type = CommandType::HELP;
        }
        // EXIT / QUIT
        else if (verb == "EXIT" || verb == "QUIT") {
            cmd.type = CommandType::EXIT;
        }
        else {
            cmd.raw = "Unknown command: " + tokens[0] + ". Type HELP for usage.";
        }
    } catch (const std::exception& e) {
        cmd.type = CommandType::UNKNOWN;
        cmd.raw  = std::string("Parse error: ") + e.what();
    }

    return cmd;
}

Command parseLine(const std::string& line) {
    return parseCommand(tokenize(line));
}

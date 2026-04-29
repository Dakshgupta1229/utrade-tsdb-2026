# TSDB — In-Memory Time-Series Database Engine

A lightweight, REPL-driven time-series data store written in C++17, inspired by systems like InfluxDB.

## Design Decisions

### Why `std::map` over `std::unordered_map`?

The core structure is `std::map<string, std::map<long long, double>>` — metric name → timestamp → value.

The inner `std::map` (balanced BST) keeps timestamps **sorted**, enabling:
- `lower_bound(t_start)` / `upper_bound(t_end)` — O(log N) range seek
- Iterator walk between bounds — O(K) where K = results

Total range query: **O(log N + K)** — optimal for time-series workloads where range queries dominate. An `unordered_map` would need O(N) scans for ranges.

### Module Separation

- **TimeSeries** — Pure data engine, no I/O knowledge
- **Parser** — Converts input strings to typed `Command` structs with `CommandType` enum
- **Exporter** — CSV file export
- **main.cpp** — Thin REPL orchestrator: read → parse → execute → print

Each module has a single responsibility, making the engine testable in isolation.

### Error Handling

- Invalid args → descriptive usage message
- Unknown commands/agg types → caught at parse time
- Non-numeric input → `std::stoll`/`std::stod` exceptions caught and shown
- Empty query results → returns empty vector (not an error)
- Empty aggregation → returns `std::nullopt` via `std::optional`

## Supported Commands

| Command | Description |
|---|---|
| `INSERT <metric> <ts> <value>` | Store a data point |
| `QUERY <metric> <t1> <t2>` | Fetch points in time range |
| `AGG <metric> <t1> <t2> <MIN\|MAX\|AVG\|COUNT>` | Aggregate over range |
| `LATEST <metric> <n>` | Most recent N points |
| `DELETE <metric>` | Remove entire metric |
| `DELETE <metric> <t1> <t2>` | Remove points in range |
| `EXPORT <metric> <t1> <t2> <file.csv>` | Export to CSV |
| `RETENTION <metric> <seconds>` | Prune old data |
| `LIST` / `COUNT <metric>` | List metrics / count points |

Use `now` keyword in place of any timestamp.

## Algorithms & Complexity

| Operation | Complexity |
|---|---|
| INSERT | O(log N) |
| QUERY / AGG | O(log N + K) |
| DELETE range | O(log N + K) |
| LATEST N | O(N) worst case |

## Trade-offs

- **Memory-only** — no persistence; a WAL or binary file would be next
- **Single-threaded** — no mutex; would need `shared_mutex` for concurrency
- **No compression** — raw doubles; real TSDBs use delta encoding
- **Unix timestamps only** — no ISO 8601 parsing

## Build & Run

```bash
# Build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j4

# Run REPL
./tsdb

# Run tests (22 tests, no external framework)
./tsdb_tests
```

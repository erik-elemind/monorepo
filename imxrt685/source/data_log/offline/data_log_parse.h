#pragma once

// Parse a data log file at the given path, and re-encode it with compressed
// EEG samples.
void data_log_parse(const char* fn);

// Stop an ongoing procedure
void data_log_parse_stop(void);

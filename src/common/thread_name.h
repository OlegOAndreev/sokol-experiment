#pragma once

#include <cstdint>

// This is a separate header from thread.h to reduce header dependencies.

// Return local_thread_pool().name() or nullptr.
const char* local_thread_pool_name();

// Return id of worker thread if called from ThreadPool task or 0.
size_t local_thread_pool_worker_id();

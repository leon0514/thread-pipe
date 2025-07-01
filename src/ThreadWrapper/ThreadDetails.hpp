#ifndef THREAD_DETAILS_HPP
#define THREAD_DETAILS_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include "ThreadWrapper/ThreadWrapperMgr.hpp" // For ThreadWrapperStatus enum

// Helper function to convert status enum to a readable string
inline const char* status_to_string(ThreadWrapperStatus status) {
    switch (status) {
        case ThreadWrapperStatus::READY:   return "READY";
        case ThreadWrapperStatus::RUNNING: return "RUNNING";
        case ThreadWrapperStatus::EXITING: return "EXITING";
        case ThreadWrapperStatus::EXITED:  return "EXITED";
        case ThreadWrapperStatus::ERROR:   return "ERROR";
        default:                           return "UNKNOWN";
    }
}

// Holds detailed information about a single thread
struct ThreadDetails {
    std::string name;
    ThreadWrapperStatus status = ThreadWrapperStatus::ERROR;
    uint32_t queue_size = 0;
    int reference_count = 0;
};

struct TaskDetails {
    std::string name;
    std::vector<ThreadDetails> threads;
};

#endif // THREAD_DETAILS_HPP
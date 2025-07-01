#ifndef THREAD_WRAPPER_ERROR_HPP
#define THREAD_WRAPPER_ERROR_HPP

enum class ThreadWrapperError {
    OK = 0,
    ERROR = 1,
    INVALID_ARGS = 2,
    ALREADY_INITED = 3,
    THREAD_ABNORMAL = 4,
    ENQUEUE_FAILED = 5,
    START_THREAD_FAILED = 6,
    ERROR_DEST_INVALID = 7,
};

using TW = ThreadWrapperError;


#endif // THREAD_WRAPPER_ERROR_HPP
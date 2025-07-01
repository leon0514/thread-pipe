#ifndef THREAD_WRAPPER_ERROR_HPP
#define THREAD_WRAPPER_ERROR_HPP

// 不再需要 <unistd.h>，因为它与错误码的定义无关

// 使用 enum class 来避免命名冲突，并提供强类型检查
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

using tw = ThreadWrapperError;


#endif // THREAD_WRAPPER_ERROR_HPP
#ifndef PARAM_HPP
#define PARAM_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

// 使用 enum class 增强类型安全和代码清晰度
enum class MessageId {
    APP_START = 1,
    CREATE_PIPELINE_MSG = 2,
    PROCESS_PIPELINE_MSG = 3,
};

// 消息体，包含路由信息和数据
// 命名更具体，反映其在管道中的作用
struct PipelineMessage
{
    std::vector<std::string> routing_slip;
    uint32_t value;
};

#endif // PARAM_HPP
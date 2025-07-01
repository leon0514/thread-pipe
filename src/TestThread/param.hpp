#ifndef PARAM_HPP
#define PARAM_HPP

namespace
{
    const int MSG_APP_START = 1;
    const int MSG_CREATE = 2;
    const int MSG_ADD = 3;
}

struct Message
{
    // 线程发送消息顺序
    std::vector<std::string> call_thread_name_list_;
    uint32_t value;
};

#endif
#ifndef THREADWRAPPERTYPE_HPP
#define THREADWRAPPERTYPE_HPP

struct ThreadWrapperMessage {
    int dest;
    int msg_id;
    std::shared_ptr<void> data = nullptr;
};

#endif
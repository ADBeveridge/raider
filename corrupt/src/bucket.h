#ifndef CORRUPT_BUCKET_H
#define CORRUPT_BUCKET_H

#include "utility.h"
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <stop_token>

class bucket
{
private:
    dev_t deviceID;

    std::vector<std::string> files;
    std::vector<std::jthread> worker_threads;

    struct strategy strategy;
    std::mutex queue_mutex;

public:
    bucket(dev_t id, struct strategy strategy);
    ~bucket();

    dev_t getDeviceID();

    bool addFile(const std::string &name);
    void shred(std::stop_token stoken);
};

#endif // CORRUPT_BUCKET_H
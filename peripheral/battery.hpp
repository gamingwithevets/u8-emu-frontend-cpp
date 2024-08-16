#pragma once
#include "../config/config.hpp"

class battery {
public:
    struct config *config;

    battery(struct config *config);
};

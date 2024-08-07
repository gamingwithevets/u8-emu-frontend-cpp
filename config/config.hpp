#include <cstdint>

class config {
public:
    int hardware_id;
    bool real_hardware;
    bool ko_mode;
    bool sample;
    bool is_5800p;
    uint8_t pd_value;
};

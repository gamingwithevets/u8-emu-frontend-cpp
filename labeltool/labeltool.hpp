#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <cstdint>

struct Label {
    std::string name;
    bool is_func;
    uint32_t parent_addr;
    std::vector<uint32_t> bcond_jmp_list;
};

std::tuple<std::map<uint32_t, Label>, std::map<uint32_t, std::string>, std::map<std::string, std::string>> load_labels(std::ifstream& f, uint32_t start);
void save_labels(std::ofstream& f, uint32_t start, std::map<uint32_t, Label>& labels, std::map<uint32_t, std::string>& data_labels, std::map<std::string, std::string>& data_bit_labels);

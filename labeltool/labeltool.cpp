// labeltool (c) 2024 GamingWithEvets Inc. Licensed under MIT
// GitHub repository: https://github.com/gamingwithevets/labeltool
// C++ conversion by ChatGPT
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <regex>
#include <string>
#include <algorithm>
#include <tuple>
#include <cstdint>
#include <format>
#include <stdexcept>
#include "labeltool.hpp"

void load_labels(std::ifstream& f, uint32_t start, std::map<uint32_t, Label>* labels/*, std::map<uint32_t, std::string>* data_labels, std::map<std::string, std::string>* data_bit_labels*/) {
    std::vector<std::vector<std::string>> label_data;

    std::string line;
    while (std::getline(f, line)) {
        std::istringstream iss(line);
        std::string word;
        std::vector<std::string> words;
        while (iss >> word) {
            word = word.substr(0, word.find("#"));
            if (word.empty()) break;
            words.push_back(word);
        }
        if (!words.empty()) {
            label_data.push_back(words);
        }
    }
    f.close();

    uint32_t curr_func = 0;
    for (const auto& data : label_data) {
        if (data.size() > 1) {
            try {
                if (data[0].rfind("f_", 0) == 0) {
                    uint32_t addr = std::stoi(data[0].substr(2), nullptr, 16) - start;
                    if (labels->find(addr) != labels->end()) {
                        std::cerr << std::format("Duplicate function label {:05X}, skipping\n", addr);
                    } else {
                        (*labels)[addr] = {data[1], true, 0, {}};
                        curr_func = addr;
                    }
                } else if (data[0].rfind(".l_", 0) == 0) {
                    uint32_t addr = curr_func + std::stoi(data[0].substr(3), nullptr, 16) - start;
                    if (labels->find(addr) != labels->end()) {
                        std::cerr << "Duplicate local label " << std::hex << curr_func << "+" << std::stoi(data[0].substr(3), nullptr, 16) << ", skipping\n";
                    } else {
                        (*labels)[addr] = {data[1], false, static_cast<uint32_t>(curr_func), {}};
                    }
                /*} else if (data[0].rfind("d_", 0) == 0) {
                    uint32_t addr = std::stoi(data[0].substr(2), nullptr, 16);
                    if (data_labels->find(addr) != data_labels->end()) {
                        std::cerr << std::format("Duplicate data label {:05X}, skipping\n", addr);
                    } else {
                        (*data_labels)[addr] = data[1];
                    }
                */} else {
                    try {
                        std::size_t idx;
                        uint32_t addr = std::stoi(data[0], &idx, 16) - start;
                        if (idx < 5) throw std::runtime_error("a");
                        if (labels->find(addr) != labels->end()) {
                            std::cerr << std::format("Duplicate function label {:05X}, skipping\n", addr);
                        } else {
                            (*labels)[addr] = {data[1], true, 0, {}};
                            curr_func = addr;
                        }
                    } catch (const std::exception&) {
                        /*if (data[0].find('.') != std::string::npos) {
                            auto s = data[0].substr(0, data[0].find('.'));
                            auto s1 = data[0].substr(s.length()+1, 1);
                            auto it = std::find_if(data_labels->begin(), data_labels->end(), [&s](const std::pair<const unsigned int, std::string>& element) { return element.second == s; });
                            if (it != data_labels->end() && std::isdigit(s1.c_str()[0])) {
                                if (0 <= std::stoi(s1) && std::stoi(s1) <= 7) {
                                    if (data_bit_labels->find(data[0]) != data_bit_labels->end()) {
                                        std::cerr << "Duplicate bit data label " << data[0] << ", skipping\n";
                                    } else {
                                        (*data_bit_labels)[data[0]] = data[1];
                                    }
                                } else {
                                    std::cerr << "Invalid bit data label " << data[0] << ", skipping\n";
                                }
                            } else {
                                std::cerr << "Invalid label " << data[0] << ", skipping\n";
                            }
                        } else {
                            std::cerr << "Invalid label " << data[0] << ", skipping\n";
                        }*/
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Exception occurred: " << e.what() << "\n";
            }
        }
    }

    return;
}

/*void save_labels(std::ofstream& f, uint32_t start, std::map<uint32_t, Label>& labels, std::map<uint32_t, std::string>& data_labels, std::map<std::string, std::string>& data_bit_labels) {
    std::map<uint32_t, Label> sorted_labels;
    for (const auto& label : labels) {
        if (label.second.is_func) {
            sorted_labels[label.first + start] = label.second;
        }
    }

    sorted_labels = std::map<uint32_t, Label>(sorted_labels.begin(), sorted_labels.end());
    data_labels = std::map<uint32_t, std::string>(data_labels.begin(), data_labels.end());
    data_bit_labels = std::map<std::string, std::string>(data_bit_labels.begin(), data_bit_labels.end());

    std::string content = "# Function + local labels\n";
    for (const auto& label : sorted_labels) {
        content += (std::ostringstream() << std::hex << label.first << "\t\t" << label.second.name << "\n").str();
    }

    content += "\n# Data labels\n";
    for (const auto& data_label : data_labels) {
        content += (std::ostringstream() << std::hex << data_label.first << "\t\t" << data_label.second << "\n").str();
    }

    content += "\n# Bit data labels\n";
    for (const auto& data_bit_label : data_bit_labels) {
        content += (std::ostringstream() << std::hex << data_bit_label.first << "\t\t" << data_bit_label.second << "\n").str();
    }

    f << content;
}
*/

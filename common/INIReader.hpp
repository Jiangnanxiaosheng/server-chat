#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

#include "Singleton.hpp"

class INIReader : public Singleton<INIReader> {
    friend class Singleton<INIReader>;  // 允许 Singleton 访问私有构造函数
public:
    class Section {
    public:
        Section(const std::map<std::string, std::string>* data) : data_(data) {}

        // 重载[]运算符用于键值访问
        std::string operator[](const std::string& key) const {
            if (!data_)
                return "";
            auto it = data_->find(key);
            return it != data_->end() ? it->second : "";
        }

        explicit operator bool() const { return data_ != nullptr; }

    private:
        const std::map<std::string, std::string>* data_;
    };

    // 加载并解析INI文件
    bool load(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        std::string line;
        std::string current_section;

        while (std::getline(file, line)) {
            trim(line);
            if (line.empty() || isComment(line)) {
                continue;
            }

            if (isSection(line)) {
                current_section = parseSection(line);
            } else {
                auto key_val = parseKeyValue(line);
                if (!key_val.first.empty()) {
                    data_[current_section][key_val.first] = key_val.second;
                }
            }
        }
        return true;
    }

    // 重载[]运算符用于节访问
    Section operator[](const std::string& section) const {
        auto it = data_.find(section);
        return it != data_.end() ? Section(&it->second) : Section(nullptr);
    }

    // 获取配置值
    std::string getValue(const std::string& section, const std::string& key,
                         const std::string& default_value = "") const {
        auto sit = data_.find(section);
        if (sit == data_.end())
            return default_value;

        auto kit = sit->second.find(key);
        if (kit == sit->second.end())
            return default_value;

        return kit->second;
    }

    void dump() {
        for (const auto& section_pair : data_) {
            std::cout << "[" << section_pair.first << "]" << std::endl;
            for (const auto& key_val_pair : section_pair.second) {
                std::cout << key_val_pair.first << " = " << key_val_pair.second << std::endl;
            }
            std::cout << std::endl;  // 打印一个空行来分隔不同的节
        }
    }

private:
    INIReader() = default;

    // 去除字符串两端空白
    static void trim(std::string& s) {
        s.erase(s.begin(),
                std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); }).base(),
                s.end());
    }

    // 判断是否是注释行
    static bool isComment(const std::string& s) { return s[0] == ';' || s[0] == '#'; }

    // 判断是否是节定义
    static bool isSection(const std::string& s) { return s[0] == '[' && s[s.size() - 1] == ']'; }

    // 解析节名
    static std::string parseSection(const std::string& s) { return s.substr(1, s.size() - 2); }

    // 解析键值对
    static std::pair<std::string, std::string> parseKeyValue(const std::string& s) {
        std::size_t eq_pos = s.find('=');
        if (eq_pos == std::string::npos)
            return {"", ""};

        std::string key = s.substr(0, eq_pos);
        std::string value = s.substr(eq_pos + 1);
        trim(key);
        trim(value);

        return {key, value};
    }

    std::map<std::string, std::map<std::string, std::string>> data_;
};
#include "config_parser.h"

#include <iostream>

ConfigParser::ConfigParser(std::string config_file) : m_config_file(config_file) {
    std::cout << "Loading '" << m_config_file << "'" << std::endl;
    m_config = YAML::LoadFile(m_config_file);
}

ConfigParser &ConfigParser::instance(std::string c) {
    static ConfigParser i(c);
    return i;
}

bool ConfigParser::has_key(const std::string &k) { return m_config[k].Type() != YAML::NodeType::Null; }

std::map<std::string, std::string> ConfigParser::config_for_key(const std::string &k) {
    std::map<std::string, std::string> result;
    if (has_key(k)) {
        YAML::Node node = m_config[k];

        if (node.Type() == YAML::NodeType::Sequence) {
            for (auto it = node.begin(); it != node.end(); ++it) {
                const YAML::Node &worker = *it;
                result.insert(std::make_pair(worker["id"].as<std::string>(), worker["port"].as<std::string>()));
            }
        }

        if (node.Type() == YAML::NodeType::Map) {
            // We now have a map node, so let's iterate through:
            for (auto it = node.begin(); it != node.end(); ++it) {
                YAML::Node key = it->first;
                YAML::Node value = it->second;
                if (value.Type() == YAML::NodeType::Scalar) {
                    result.insert(std::make_pair(key.as<std::string>(), value.as<std::string>()));
                }
                if (value.Type() == YAML::NodeType::Map) {
                    // This should be true; do something here with the map.
                    std::string s = std::string("Nested map found for key '");
                    s += k;
                    s += "'";
                    std::cout << s << std::endl;
                }
                if (value.Type() == YAML::NodeType::Sequence) {
                    std::cout << value << std::endl;
                }
            }
        }

    } else {
        std::string err = std::string("No such key '");
        err += k;
        err += "'";
        std::cerr << err << std::endl;
    }

    return result;
}

std::map<std::string, std::string> ConfigParser::Server() { return config_for_key("server"); }

std::map<std::string, std::string> ConfigParser::Workers() { return config_for_key("workers"); }

std::map<std::string, std::string> ConfigParser::kafka() { return config_for_key("kafka"); }

ConfigParser::~ConfigParser(){};
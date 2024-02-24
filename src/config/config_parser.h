#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_

#include <yaml-cpp/yaml.h>

#include <map>
#include <string>

class ConfigParser {
   private:
    ConfigParser(std::string c);
    std::string m_config_file;
    YAML::Node m_config;
    std::map<std::string, std::string> config_for_key(const std::string &key);

   public:
    /*
    Deleted functions should generally
    be public as it results in better error messages
    due to the compilers behavior to check accessibility
    before deleted status
    */
    ConfigParser(const ConfigParser &) = delete;
    void operator=(const ConfigParser &) = delete;

    static ConfigParser &instance(std::string c);
    bool has_key(const std::string &k);
    std::map<std::string, std::string> Server();
    std::map<std::string, std::string> Workers();
    std::map<std::string, std::string> kafka();
    ~ConfigParser();
};
#endif
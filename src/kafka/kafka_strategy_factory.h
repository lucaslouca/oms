#ifndef KAFKA_FACTORY_H
#define KAFKA_FACTORY_H

#include <map>
#include <memory>
#include <string>

#include "../lock_free_queue.h"
#include "kafka_message_strategy.h"

#define REGISTER_DEC_TYPE(TYPE_NAME) static StrategyRegister<TYPE_NAME> reg

#define REGISTER_DEF_TYPE(TYPE_NAME, CONFIG_NAME) StrategyRegister<TYPE_NAME> TYPE_NAME::reg(#CONFIG_NAME)

/*
If KafkaMessageStrategy is a base of T then std::is_base_of_v<A, T> is true and
std::enable_if_t has a public member typedef type 'bool' and we give it the value of true.
where bool = true is just an unnamed non-type template parameter with the value of true.
The reason we do = true is so that it has a default value and we don't need to pass a value to it.

If KafkaMessageStrategy is not a base of T then the condition is false and std::enable_if_t
has no member typedef called type. This means, whenever the implementation tries to access
enable_if<B,T>::type when B = false, the compiler will raise compilation error, as the object
member type is not defined.

https://leimao.github.io/blog/CPP-Enable-If/
*/
template <typename T, std::enable_if_t<std::is_base_of_v<KafkaMessageStrategy, T>, bool> = true>
std::unique_ptr<KafkaMessageStrategy> CreateT(std::string name) {
    return std::make_unique<T>(name);
}

struct KafkaFactory {
    template <typename T>
    using MFP = std::unique_ptr<T> (*)(std::string);
    using TypeMap = std::map<std::string, MFP<KafkaMessageStrategy>>;

   private:
    static TypeMap *map;

   protected:
    static TypeMap *GetMap() {
        // Never deleted. Exists until program termination.
        if (!map) {
            map = new TypeMap;
        }
        return map;
    }

   public:
    static std::unique_ptr<KafkaMessageStrategy> GetInstance(const std::string &s, const std::string name) {
        TypeMap::iterator it = GetMap()->find(s);
        if (it == GetMap()->end()) {
            return nullptr;
        } else {
            return it->second(name);
        }
    }
};

template <typename T>
struct StrategyRegister : KafkaFactory {
    StrategyRegister(const std::string &type) {
        GetMap()->insert(
            std::make_pair<std::string, MFP<KafkaMessageStrategy>>(static_cast<std::string>(type), &CreateT<T>));
    }
};

#endif
#include <boost/uuid.hpp>
#include <iostream>
#include <string>

std::string generate_unique_string() {
    // 创建UUID对象
    boost::uuids::uuid uuid = boost::uuids::random_generator()();

    // 将UUID转换为字符串
    std::string unique_string = to_string(uuid);

    return unique_string;
}

int main() { std::cout << generate_unique_string() << "\n"; }
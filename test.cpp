#include <unordered_map>
#include <string>
#include <iostream>

int main() {
    std::unordered_map<std::string, std::string> clients;
    clients["user1"] = "password1";
    std::cout << "Client DB initialized." << std::endl;
    return 0;
}

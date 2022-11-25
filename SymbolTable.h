#include <string>
#include <vector>
#include <unordered_map>

#ifndef LAB2_SYMBOLTABLE_H
#define LAB2_SYMBOLTABLE_H

template <typename T>
class SymbolTable
{
public:
    void createNewEnv();
    bool addToEnv(std::string id, T val);
    T getFromEnv(std::string id);
    void popEnv();
    bool checkEnv(std::string id);

public: // for debugging now
    std::vector<std::unordered_map<std::string, T>> table;
};

#endif // LAB2_SYMBOLTABLE_H

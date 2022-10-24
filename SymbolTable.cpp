#include "SymbolTable.h"
#include "ast.h"

template <typename T>
void SymbolTable<T>::createNewEnv() {
    this->table.push_back(std::unordered_map<std::string, T>());
}

template <typename T>
void SymbolTable<T>::popEnv() {
    this->table.pop_back();
}

template <typename T>
bool SymbolTable<T>::addToEnv(std::string id, T val) {
    if (this->table.back().count(id) > 0) {
        return false;
    }
    this->table.back()[id] = val;
    return true;
}

template <typename T>
bool SymbolTable<T>::checkEnv(std::string id) {
    return this->table.back().count(id) > 0;
}

template <typename T>
T SymbolTable<T>::getFromEnv(std::string id) {
    for (auto it = this->table.rbegin(); it != this->table.rend(); it++) {
        if (it->count(id) != 0) {
            return (*it)[id];
        }
    }
    return nullptr;
}

template class SymbolTable<ast::yyAST*>;
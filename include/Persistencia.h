#ifndef PERSISTENCIA_H
#define PERSISTENCIA_H

#include <string>

#include "Estoque.h"

class Persistencia {
public:
    static bool salvar(
        const std::string& caminho,
        const Estoque& estoque,
        std::string& erro);

    static bool carregar(
        const std::string& caminho,
        Estoque& estoque,
        std::string& erro);
};

#endif

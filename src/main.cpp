#include <iostream>
#include <string>

#include "Estoque.h"

int main() {
    Estoque estoque;
    std::string opcao;

    while (true) {
        std::cout << "=== StockManager ===\n";
        std::cout << "1. Cadastrar produto\n";
        std::cout << "2. Buscar produto\n";
        std::cout << "0. Sair\n";
        std::cout << "Opcao: ";

        if (!std::getline(std::cin, opcao)) {
            break;
        }

        if (opcao == "0") {
            break;
        } else if (opcao == "1" || opcao == "2") {
            std::cout << "Funcionalidade ainda nao implementada.\n";
        } else {
            std::cout << "Opcao invalida. Tente novamente.\n";
        }
    }

    return 0;
}

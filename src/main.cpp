#include <charconv>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

#include "Estoque.h"

bool converterPrecoParaCentavos(const std::string& texto, long long& centavos) {
    if (texto.empty()) {
        return false;
    }

    const std::size_t posicaoVirgula = texto.find(',');
    if (posicaoVirgula != std::string::npos &&
        texto.find(',', posicaoVirgula + 1) != std::string::npos) {
        return false;
    }

    const std::size_t tamanhoReais =
        posicaoVirgula == std::string::npos ? texto.size() : posicaoVirgula;
    if (tamanhoReais == 0) {
        return false;
    }

    std::size_t tamanhoCentavos = 0;
    if (posicaoVirgula != std::string::npos) {
        tamanhoCentavos = texto.size() - posicaoVirgula - 1;
        if (tamanhoCentavos == 0 || tamanhoCentavos > 2) {
            return false;
        }
    }

    const long long limite = std::numeric_limits<long long>::max();
    long long reais = 0;
    for (std::size_t i = 0; i < tamanhoReais; ++i) {
        const char caractere = texto[i];
        if (caractere < '0' || caractere > '9') {
            return false;
        }

        const int digito = caractere - '0';
        if (reais > (limite - digito) / 10) {
            return false;
        }
        reais = reais * 10 + digito;
    }

    long long parteCentavos = 0;
    if (tamanhoCentavos > 0) {
        const char primeiroDigito = texto[posicaoVirgula + 1];
        if (primeiroDigito < '0' || primeiroDigito > '9') {
            return false;
        }
        parteCentavos = (primeiroDigito - '0') * 10;

        if (tamanhoCentavos == 2) {
            const char segundoDigito = texto[posicaoVirgula + 2];
            if (segundoDigito < '0' || segundoDigito > '9') {
                return false;
            }
            parteCentavos += segundoDigito - '0';
        }
    }

    if (reais > (limite - parteCentavos) / 100) {
        return false;
    }

    const long long resultado = reais * 100 + parteCentavos;
    centavos = resultado;
    return true;
}

bool converterQuantidade(const std::string& texto, int& quantidade) {
    if (texto.empty()) {
        return false;
    }

    for (char caractere : texto) {
        if (caractere < '0' || caractere > '9') {
            return false;
        }
    }

    int resultado = 0;
    const char* inicio = texto.data();
    const char* fim = inicio + texto.size();
    const auto conversao = std::from_chars(inicio, fim, resultado);

    if (conversao.ec != std::errc{} || conversao.ptr != fim) {
        return false;
    }

    quantidade = resultado;
    return true;
}

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




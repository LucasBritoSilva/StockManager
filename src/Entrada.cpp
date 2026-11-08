#include "Entrada.h"

#include <ctime>
#include <charconv>
#include <limits>

namespace {
bool formatoDataValido(const std::string& data) {
    if (data.size() != 20 || data[4] != '-' || data[7] != '-' ||
        data[10] != 'T' || data[13] != ':' || data[16] != ':' || data[19] != 'Z') {
        return false;
    }
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16 || i == 19) continue;
        if (data[i] < '0' || data[i] > '9') return false;
    }
    const auto numero = [&](std::size_t inicio, std::size_t tamanho) {
        int valor = 0;
        const char* primeiro = data.data() + inicio;
        const char* ultimo = primeiro + tamanho;
        const auto conversao = std::from_chars(primeiro, ultimo, valor);
        return conversao.ec == std::errc{} && conversao.ptr == ultimo ? valor : -1;
    };
    const int ano = numero(0, 4);
    const int mes = numero(5, 2);
    const int dia = numero(8, 2);
    const int hora = numero(11, 2);
    const int minuto = numero(14, 2);
    const int segundo = numero(17, 2);
    if (ano < 1 || mes < 1 || mes > 12 || hora < 0 || hora > 23 ||
        minuto < 0 || minuto > 59 || segundo < 0 || segundo > 59) return false;
    const bool bissexto = ano % 400 == 0 || (ano % 4 == 0 && ano % 100 != 0);
    const int diasNoMes[] = {31, bissexto ? 29 : 28, 31, 30, 31, 30,
                             31, 31, 30, 31, 30, 31};
    return dia >= 1 && dia <= diasNoMes[mes - 1];
}
}

bool validarEntrada(const Entrada& entrada) {
    if (entrada.identificador <= 0 || entrada.codigoProduto.empty() ||
        entrada.nomeProduto.empty() || entrada.quantidade <= 0 ||
        entrada.quantidadeAnterior < 0 || entrada.quantidadePosterior < 0 ||
        !formatoDataValido(entrada.dataHoraUtc)) {
        return false;
    }

    if (entrada.tipo == TipoMovimentacao::entrada) {
        if (entrada.quantidadeAnterior >
            std::numeric_limits<int>::max() - entrada.quantidade) return false;
        return entrada.quantidadePosterior ==
               entrada.quantidadeAnterior + entrada.quantidade;
    }
    if (entrada.tipo == TipoMovimentacao::saida) {
        if (entrada.quantidade > entrada.quantidadeAnterior) return false;
        return entrada.quantidadePosterior ==
               entrada.quantidadeAnterior - entrada.quantidade;
    }
    return false;
}

std::string dataHoraUtcAtual() {
    const std::time_t agora = std::time(nullptr);
    const std::tm* utc = std::gmtime(&agora);
    if (utc == nullptr) return {};
    char texto[21]{};
    if (std::strftime(texto, sizeof(texto), "%Y-%m-%dT%H:%M:%SZ", utc) == 0) return {};
    return texto;
}

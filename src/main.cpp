#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <system_error>

#include "Estoque.h"
#include "Persistencia.h"

bool salvarAlteracao(const Estoque& estoque);

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

bool cadastrarProdutoPeloTerminal(Estoque& estoque) {
    std::string codigo;
    while (true) {
        std::cout << "Codigo: ";
        if (!std::getline(std::cin, codigo)) {
            return false;
        }
        if (!codigo.empty()) {
            break;
        }
        std::cout << "Codigo nao pode ser vazio. Tente novamente.\n";
    }

    std::string nome;
    while (true) {
        std::cout << "Nome: ";
        if (!std::getline(std::cin, nome)) {
            return false;
        }
        if (!nome.empty()) {
            break;
        }
        std::cout << "Nome nao pode ser vazio. Tente novamente.\n";
    }

    std::string textoPreco;
    long long precoCentavos = 0;
    while (true) {
        std::cout << "Preco (ex.: 159,90): ";
        if (!std::getline(std::cin, textoPreco)) {
            return false;
        }
        if (converterPrecoParaCentavos(textoPreco, precoCentavos)) {
            break;
        }
        std::cout << "Preco invalido. Tente novamente.\n";
    }

    std::string textoQuantidade;
    int quantidade = 0;
    while (true) {
        std::cout << "Quantidade: ";
        if (!std::getline(std::cin, textoQuantidade)) {
            return false;
        }
        if (converterQuantidade(textoQuantidade, quantidade)) {
            break;
        }
        std::cout << "Quantidade invalida. Tente novamente.\n";
    }

    const Produto produto{codigo, nome, precoCentavos, quantidade};
    if (!validarProduto(produto)) {
        std::cout << "Dados do produto invalidos.\n";
        return true;
    }

    if (estoque.buscarProdutoPorCodigo(produto.codigo) != nullptr) {
        std::cout << "Ja existe um produto com esse codigo.\n";
        return true;
    }

    if (estoque.cadastrarProduto(produto)) {
        if (salvarAlteracao(estoque)) {
            std::cout << "Produto cadastrado e salvo com sucesso.\n";
        }
    } else {
        std::cout << "Nao foi possivel cadastrar o produto.\n";
    }

    return true;
}
bool buscarProdutoPeloTerminal(Estoque& estoque) {
    std::string codigo;
    while (true) {
        std::cout << "Codigo do produto: ";
        if (!std::getline(std::cin, codigo)) {
            return false;
        }
        if (!codigo.empty()) {
            break;
        }
        std::cout << "Codigo nao pode ser vazio. Tente novamente.\n";
    }

    const Produto* produto = estoque.buscarProdutoPorCodigo(codigo);
    if (produto == nullptr) {
        std::cout << "Produto nao encontrado.\n";
        return true;
    }

    std::cout << "Codigo: " << produto->codigo << '\n';
    std::cout << "Nome: " << produto->nome << '\n';

    const char preenchimentoOriginal = std::cout.fill();
    std::cout << "Preco: R$ " << produto->precoCentavos / 100 << ','
              << std::setw(2) << std::setfill('0')
              << produto->precoCentavos % 100 << '\n';
    std::cout.fill(preenchimentoOriginal);

    std::cout << "Quantidade: " << produto->quantidade << '\n';
    return true;
}
void listarProdutosPeloTerminal(const Estoque& estoque) {
    const std::vector<Produto>& produtos = estoque.obterProdutos();
    if (produtos.empty()) {
        std::cout << "Estoque vazio.\n";
        std::cout << "Total de produtos: 0\n";
        return;
    }

    const auto formatarPreco = [](long long precoCentavos) {
        std::string preco = "R$ " + std::to_string(precoCentavos / 100) + ",";
        const long long centavos = precoCentavos % 100;
        if (centavos < 10) {
            preco += '0';
        }
        preco += std::to_string(centavos);
        return preco;
    };

    std::size_t larguraCodigo = std::string("Codigo").size();
    std::size_t larguraNome = std::string("Nome").size();
    std::size_t larguraPreco = std::string("Preco").size();
    std::size_t larguraQuantidade = std::string("Quantidade").size();

    for (const Produto& produto : produtos) {
        larguraCodigo = std::max(larguraCodigo, produto.codigo.size());
        larguraNome = std::max(larguraNome, produto.nome.size());
        larguraPreco = std::max(larguraPreco, formatarPreco(produto.precoCentavos).size());
        larguraQuantidade = std::max(larguraQuantidade,
                                     std::to_string(produto.quantidade).size());
    }

    const std::ios_base::fmtflags flagsOriginais = std::cout.flags();
    const char preenchimentoOriginal = std::cout.fill();

    std::cout << std::left << std::setw(static_cast<int>(larguraCodigo)) << "Codigo" << "  "
              << std::left << std::setw(static_cast<int>(larguraNome)) << "Nome" << "  "
              << std::right << std::setw(static_cast<int>(larguraPreco)) << "Preco" << "  "
              << std::right << std::setw(static_cast<int>(larguraQuantidade)) << "Quantidade"
              << '\n';

    for (const Produto& produto : produtos) {
        std::cout << std::left << std::setw(static_cast<int>(larguraCodigo)) << produto.codigo
                  << "  " << std::left << std::setw(static_cast<int>(larguraNome))
                  << produto.nome << "  " << std::right
                  << std::setw(static_cast<int>(larguraPreco))
                  << formatarPreco(produto.precoCentavos) << "  "
                  << std::right << std::setw(static_cast<int>(larguraQuantidade))
                  << produto.quantidade << '\n';
    }

    std::cout.flags(flagsOriginais);
    std::cout.fill(preenchimentoOriginal);
    std::cout << "Total de produtos: " << produtos.size() << '\n';
}
bool movimentarProdutoPeloTerminal(Estoque& estoque, bool entrada) {
    std::string codigo;
    while (true) {
        std::cout << "Codigo do produto: ";
        if (!std::getline(std::cin, codigo)) {
            return false;
        }
        if (!codigo.empty()) {
            break;
        }
        std::cout << "Codigo nao pode ser vazio. Tente novamente.\n";
    }

    std::string textoQuantidade;
    int quantidade = 0;
    while (true) {
        std::cout << (entrada ? "Quantidade da entrada: " : "Quantidade da saida: ");
        if (!std::getline(std::cin, textoQuantidade)) {
            return false;
        }
        if (converterQuantidade(textoQuantidade, quantidade)) {
            break;
        }
        std::cout << "Quantidade invalida. Digite um inteiro nao negativo.\n";
    }

    const ResultadoMovimentacao resultado = entrada
        ? estoque.registrarEntrada(codigo, quantidade)
        : estoque.registrarSaida(codigo, quantidade);

    switch (resultado) {
        case ResultadoMovimentacao::sucesso:
            if (salvarAlteracao(estoque)) {
                std::cout << (entrada ? "Entrada registrada e salva com sucesso.\n"
                                      : "Saida registrada e salva com sucesso.\n");
            }
            break;
        case ResultadoMovimentacao::produtoNaoEncontrado:
            std::cout << "Produto nao encontrado.\n";
            break;
        case ResultadoMovimentacao::quantidadeInvalida:
            std::cout << "Quantidade da movimentacao deve ser maior que zero.\n";
            break;
        case ResultadoMovimentacao::limiteEstoqueExcedido:
            std::cout << "Entrada excede o limite maximo de estoque.\n";
            break;
        case ResultadoMovimentacao::estoqueInsuficiente:
            std::cout << "Estoque insuficiente para essa saida.\n";
            break;
    }

    return true;
}

bool registrarEntradaPeloTerminal(Estoque& estoque) {
    return movimentarProdutoPeloTerminal(estoque, true);
}

bool registrarSaidaPeloTerminal(Estoque& estoque) {
    return movimentarProdutoPeloTerminal(estoque, false);
}

bool salvarAlteracao(const Estoque& estoque) {
    std::string erro;
    if (!Persistencia::salvar("data/produtos.csv", estoque, erro)) {
        std::cerr << "Aviso: alteracao mantida apenas em memoria; nao foi persistida: "
                  << erro << '\n';
        return false;
    }
    if (!erro.empty()) {
        std::cerr << "Aviso: " << erro << '\n';
    }
    return true;
}

int main() {
    Estoque estoque;
    std::string erroCarregamento;
    if (!Persistencia::carregar("data/produtos.csv", estoque, erroCarregamento)) {
        std::cerr << "Erro ao carregar os dados; o arquivo nao sera sobrescrito: "
                  << erroCarregamento << '\n';
        return 1;
    }
    std::string opcao;

    while (true) {
        std::cout << "=== StockManager ===\n";
        std::cout << "1. Cadastrar produto\n";
        std::cout << "2. Buscar produto\n";
        std::cout << "3. Listar produtos\n";
        std::cout << "4. Registrar entrada\n";
        std::cout << "5. Registrar saida\n";
        std::cout << "0. Sair\n";
        std::cout << "Opcao: ";

        if (!std::getline(std::cin, opcao)) {
            break;
        }

        if (opcao == "0") {
            break;
        } else if (opcao == "1") {
            if (!cadastrarProdutoPeloTerminal(estoque)) {
                break;
            }
        } else if (opcao == "2") {
            if (!buscarProdutoPeloTerminal(estoque)) {
                break;
            }
        } else if (opcao == "3") {
            listarProdutosPeloTerminal(estoque);
        } else if (opcao == "4") {
            if (!registrarEntradaPeloTerminal(estoque)) {
                break;
            }
        } else if (opcao == "5") {
            if (!registrarSaidaPeloTerminal(estoque)) {
                break;
            }
        } else {
            std::cout << "Opcao invalida. Tente novamente.\n";
        }
    }

    return 0;
}


#include <algorithm>
#include <charconv>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <system_error>
#include <vector>

#include "Estoque.h"
#include "Persistencia.h"
#include "Relatorios.h"
#include "Historico.h"

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

    const Estoque estoqueAnterior = estoque;
    if (estoque.cadastrarProduto(produto)) {
        if (salvarAlteracao(estoque)) {
            std::cout << "Produto cadastrado e salvo com sucesso.\n";
        } else {
            estoque = estoqueAnterior;
            std::cout << "Cadastro cancelado; estoque anterior restaurado.\n";
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
bool movimentarProdutoPeloTerminal(Estoque& estoque,
                                  Historico& historico,
                                  bool entrada) {
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

    const Produto* produtoEncontrado = estoque.buscarProdutoPorCodigo(codigo);
    if (produtoEncontrado == nullptr) {
        std::cout << "Produto nao encontrado.\n";
        return true;
    }

    const Produto produtoAntes = *produtoEncontrado;
    const Estoque estoqueAnterior = estoque;
    const Historico historicoAnterior = historico;
    const ResultadoMovimentacao resultado = entrada
        ? estoque.registrarEntrada(codigo, quantidade)
        : estoque.registrarSaida(codigo, quantidade);

    switch (resultado) {
        case ResultadoMovimentacao::sucesso:
        {
            Entrada movimento;
            movimento.identificador = historico.proximoIdentificador();
            movimento.codigoProduto = produtoAntes.codigo;
            movimento.nomeProduto = produtoAntes.nome;
            movimento.tipo = entrada ? TipoMovimentacao::entrada : TipoMovimentacao::saida;
            movimento.quantidade = quantidade;
            movimento.dataHoraUtc = dataHoraUtcAtual();
            movimento.quantidadeAnterior = produtoAntes.quantidade;
            movimento.quantidadePosterior =
                estoque.buscarProdutoPorCodigo(codigo)->quantidade;
            std::string erro;
            if (movimento.identificador == 0 || !historico.adicionar(movimento)) {
                estoque = estoqueAnterior;
                historico = historicoAnterior;
                std::cout << "Movimentacao cancelada: nao foi possivel criar o registro historico.\n";
                break;
            }
            if (!Persistencia::salvarHistorico("data/movimentacoes.csv", historico, erro)) {
                estoque = estoqueAnterior;
                historico = historicoAnterior;
                std::cout << "Movimentacao cancelada; historico nao persistido: "
                          << erro << '\n';
                break;
            }
            if (!Persistencia::salvar("data/produtos.csv", estoque, erro)) {
                estoque = estoqueAnterior;
                historico = historicoAnterior;
                std::string erroRestauracao;
                if (!Persistencia::salvarHistorico("data/movimentacoes.csv",
                                                    historico, erroRestauracao)) {
                    std::cout << "ERRO: estoque nao salvo e historico nao restaurado: "
                              << erro << "; " << erroRestauracao << '\n';
                } else {
                    std::cout << "Movimentacao cancelada; dados nao persistidos: "
                              << erro << '\n';
                }
                break;
            }
            std::cout << (entrada ? "Entrada registrada e salva com sucesso.\n"
                                  : "Saida registrada e salva com sucesso.\n");
            break;
        }
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

bool registrarEntradaPeloTerminal(Estoque& estoque, Historico& historico) {
    return movimentarProdutoPeloTerminal(estoque, historico, true);
}

bool registrarSaidaPeloTerminal(Estoque& estoque, Historico& historico) {
    return movimentarProdutoPeloTerminal(estoque, historico, false);
}

bool salvarAlteracao(const Estoque& estoque) {
    std::string erro;
    if (!Persistencia::salvar("data/produtos.csv", estoque, erro)) {
        std::cerr << "Aviso: nao foi possivel persistir a alteracao: "
                  << erro << '\n';
        return false;
    }
    if (!erro.empty()) {
        std::cerr << "Aviso: " << erro << '\n';
    }
    return true;
}

std::string formatarCentavos(long long valor) {
    std::string texto = "R$ " + std::to_string(valor / 100) + ",";
    const long long centavos = valor % 100;
    if (centavos < 10) texto += '0';
    texto += std::to_string(centavos);
    return texto;
}

bool editarProdutoPeloTerminal(Estoque& estoque) {
    std::string codigoAtual;
    while (true) {
        std::cout << "Codigo atual do produto: ";
        if (!std::getline(std::cin, codigoAtual)) return false;
        if (!codigoAtual.empty()) break;
        std::cout << "Codigo nao pode ser vazio. Tente novamente.\n";
    }
    const Produto* encontrado = estoque.buscarProdutoPorCodigo(codigoAtual);
    if (encontrado == nullptr) {
        std::cout << "Produto nao encontrado.\n";
        return true;
    }
    const Produto anterior = *encontrado;

    std::string novoCodigo;
    std::cout << "Novo codigo (Enter mantem " << anterior.codigo << "): ";
    if (!std::getline(std::cin, novoCodigo)) return false;
    if (novoCodigo.empty()) novoCodigo = anterior.codigo;

    std::string novoNome;
    std::cout << "Novo nome (Enter mantem o atual): ";
    if (!std::getline(std::cin, novoNome)) return false;
    if (novoNome.empty()) novoNome = anterior.nome;

    long long novoPreco = anterior.precoCentavos;
    while (true) {
        std::string texto;
        std::cout << "Novo preco (Enter mantem o atual): ";
        if (!std::getline(std::cin, texto)) return false;
        if (texto.empty() || converterPrecoParaCentavos(texto, novoPreco)) break;
        std::cout << "Preco invalido. Tente novamente.\n";
    }

    int novoMinimo = anterior.quantidadeMinima;
    while (true) {
        std::string texto;
        std::cout << "Nova quantidade minima (Enter mantem o atual): ";
        if (!std::getline(std::cin, texto)) return false;
        if (texto.empty() || converterQuantidade(texto, novoMinimo)) break;
        std::cout << "Quantidade minima invalida. Tente novamente.\n";
    }

    if (novoCodigo == anterior.codigo && novoNome == anterior.nome &&
        novoPreco == anterior.precoCentavos && novoMinimo == anterior.quantidadeMinima) {
        std::cout << "Nenhuma alteracao realizada.\n";
        return true;
    }

    const Estoque estoqueAnterior = estoque;
    const ResultadoProduto resultado = estoque.editarProduto(
        codigoAtual, novoCodigo, novoNome, novoPreco, novoMinimo);
    if (resultado == ResultadoProduto::produtoNaoEncontrado) {
        std::cout << "Produto nao encontrado.\n";
    } else if (resultado == ResultadoProduto::codigoDuplicado) {
        std::cout << "Esse codigo ja pertence a outro produto.\n";
    } else if (resultado != ResultadoProduto::sucesso) {
        std::cout << "Dados do produto invalidos.\n";
    } else if (salvarAlteracao(estoque)) {
        std::cout << "Produto atualizado e salvo. Quantidade em estoque preservada: "
                  << anterior.quantidade << ".\n";
    } else {
        estoque = estoqueAnterior;
        std::cout << "Edicao cancelada; dados anteriores restaurados.\n";
    }
    return true;
}

bool excluirProdutoPeloTerminal(Estoque& estoque) {
    std::string codigo;
    std::cout << "Codigo do produto a excluir: ";
    if (!std::getline(std::cin, codigo)) return false;
    if (codigo.empty()) {
        std::cout << "Codigo nao pode ser vazio.\n";
        return true;
    }
    const Produto* produto = estoque.buscarProdutoPorCodigo(codigo);
    if (produto == nullptr) {
        std::cout << "Produto nao encontrado.\n";
        return true;
    }
    std::cout << "Excluir '" << produto->nome << "' (" << produto->codigo
              << ")? Digite S para confirmar: ";
    std::string confirmacao;
    if (!std::getline(std::cin, confirmacao)) return false;
    if (confirmacao != "S" && confirmacao != "s" &&
        confirmacao != "SIM" && confirmacao != "sim") {
        std::cout << "Exclusao cancelada.\n";
        return true;
    }
    const Estoque estoqueAnterior = estoque;
    if (estoque.excluirProduto(codigo) == ResultadoProduto::sucesso) {
        if (salvarAlteracao(estoque)) {
            std::cout << "Produto excluido. O historico de movimentacoes foi preservado.\n";
        } else {
            estoque = estoqueAnterior;
            std::cout << "Exclusao cancelada; produto restaurado.\n";
        }
    }
    return true;
}

void mostrarProdutosDoMinimo(const std::vector<Produto>& produtos, const char* titulo) {
    std::cout << titulo << '\n';
    if (produtos.empty()) {
        std::cout << "  Nenhum produto.\n";
        return;
    }
    for (const Produto& produto : produtos) {
        std::cout << "  " << produto.codigo << " | " << produto.nome
                  << " | disponivel: " << produto.quantidade
                  << " | minimo: " << produto.quantidadeMinima << '\n';
    }
}

void consultarEstoqueBaixo(const Estoque& estoque, const Historico& historico) {
    const RelatorioEstoque relatorio = gerarRelatorioEstoque(estoque, historico);
    mostrarProdutosDoMinimo(relatorio.abaixoDoMinimo, "Abaixo do minimo:");
    mostrarProdutosDoMinimo(relatorio.noMinimo, "No limite minimo:");
    mostrarProdutosDoMinimo(relatorio.semUnidades, "Sem unidades:");
}

void mostrarRelatorios(const Estoque& estoque, const Historico& historico) {
    const RelatorioEstoque relatorio = gerarRelatorioEstoque(estoque, historico);
    std::cout << "=== Relatorios ===\n";
    std::cout << "Produtos cadastrados: " << relatorio.totalProdutos << '\n';
    if (relatorio.totalUnidadesOverflow) std::cout << "Unidades em estoque: overflow\n";
    else std::cout << "Unidades em estoque: " << relatorio.totalUnidades << '\n';
    if (relatorio.valorTotalOverflow) std::cout << "Valor estimado: overflow\n";
    else std::cout << "Valor estimado: " << formatarCentavos(relatorio.valorTotalCentavos) << '\n';
    std::cout << "Abaixo do minimo: " << relatorio.abaixoDoMinimo.size() << '\n';
    std::cout << "No minimo: " << relatorio.noMinimo.size() << '\n';
    std::cout << "Sem unidades: " << relatorio.semUnidades.size() << '\n';
    std::cout << "Maior quantidade:";
    if (relatorio.maiorQuantidade.empty()) std::cout << " nenhum produto\n";
    else {
        std::cout << '\n';
        for (const Produto& produto : relatorio.maiorQuantidade) {
            std::cout << "  " << produto.codigo << " | " << produto.nome
                      << " | " << produto.quantidade << " unidades\n";
        }
    }
    std::cout << "Movimentacoes: " << relatorio.totalEntradas << " entradas / "
              << relatorio.totalSaidas << " saidas\n";
    std::cout << "Unidades de entrada: ";
    if (relatorio.unidadesEntradasOverflow) std::cout << "overflow\n";
    else std::cout << relatorio.unidadesEntradas << '\n';
    std::cout << "Unidades de saida: ";
    if (relatorio.unidadesSaidasOverflow) std::cout << "overflow\n";
    else std::cout << relatorio.unidadesSaidas << '\n';
}

void mostrarHistorico(const Historico& historico) {
    if (historico.obterEntradas().empty()) {
        std::cout << "Historico vazio.\n";
        return;
    }
    for (const Entrada& entrada : historico.obterEntradas()) {
        std::cout << '#' << entrada.identificador << " | " << entrada.dataHoraUtc << " | "
                  << (entrada.tipo == TipoMovimentacao::entrada ? "ENTRADA" : "SAIDA")
                  << " | " << entrada.codigoProduto << " | " << entrada.nomeProduto
                  << " | " << entrada.quantidadeAnterior << " -> "
                  << entrada.quantidadePosterior << " (mov. "
                  << entrada.quantidade << ")\n";
    }
}

bool reconciliarHistoricoPendente(Estoque& estoque,
                                  const Historico& historico,
                                  std::string& erro) {
    std::map<std::string, const Entrada*> ultimaPorCodigo;
    for (const Entrada& entrada : historico.obterEntradas()) {
        ultimaPorCodigo[entrada.codigoProduto] = &entrada;
    }

    bool alterado = false;
    for (const auto& par : ultimaPorCodigo) {
        const Entrada& entrada = *par.second;
        const Produto* produto = estoque.buscarProdutoPorCodigo(entrada.codigoProduto);
        if (produto == nullptr || produto->quantidade == entrada.quantidadePosterior) continue;
        if (produto->quantidade != entrada.quantidadeAnterior) {
            erro = "historico inconsistente para o codigo " + entrada.codigoProduto;
            return false;
        }
        const ResultadoMovimentacao resultado = entrada.tipo == TipoMovimentacao::entrada
            ? estoque.registrarEntrada(entrada.codigoProduto, entrada.quantidade)
            : estoque.registrarSaida(entrada.codigoProduto, entrada.quantidade);
        if (resultado != ResultadoMovimentacao::sucesso) {
            erro = "nao foi possivel recuperar a movimentacao " +
                   std::to_string(entrada.identificador);
            return false;
        }
        alterado = true;
    }
    if (alterado && !Persistencia::salvar("data/produtos.csv", estoque, erro)) {
        erro = "recuperacao de movimentacao nao persistida: " + erro;
        return false;
    }
    if (alterado) std::cout << "Movimentacao pendente recuperada do historico.\n";
    return true;
}

int main() {
    Estoque estoque;
    Historico historico;
    std::string erroCarregamento;
    if (!Persistencia::carregar("data/produtos.csv", estoque, erroCarregamento)) {
        std::cerr << "Erro ao carregar os dados; o arquivo nao sera sobrescrito: "
                  << erroCarregamento << '\n';
        return 1;
    }
    if (!Persistencia::carregarHistorico("data/movimentacoes.csv", historico,
                                         erroCarregamento)) {
        std::cerr << "Erro ao carregar o historico; nenhum dado sera sobrescrito: "
                  << erroCarregamento << '\n';
        return 1;
    }
    if (!reconciliarHistoricoPendente(estoque, historico, erroCarregamento)) {
        std::cerr << "Erro de consistencia entre estoque e historico: "
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
        std::cout << "6. Editar produto\n";
        std::cout << "7. Excluir produto\n";
        std::cout << "8. Consultar estoque baixo\n";
        std::cout << "9. Relatorios\n";
        std::cout << "10. Historico de movimentacoes\n";
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
            if (!registrarEntradaPeloTerminal(estoque, historico)) {
                break;
            }
        } else if (opcao == "5") {
            if (!registrarSaidaPeloTerminal(estoque, historico)) {
                break;
            }
        } else if (opcao == "6") {
            if (!editarProdutoPeloTerminal(estoque)) break;
        } else if (opcao == "7") {
            if (!excluirProdutoPeloTerminal(estoque)) break;
        } else if (opcao == "8") {
            consultarEstoqueBaixo(estoque, historico);
        } else if (opcao == "9") {
            mostrarRelatorios(estoque, historico);
        } else if (opcao == "10") {
            mostrarHistorico(historico);
        } else {
            std::cout << "Opcao invalida. Tente novamente.\n";
        }
    }

    return 0;
}


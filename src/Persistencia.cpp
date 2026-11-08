#include "Persistencia.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <atomic>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

struct RegistroCsv {
    std::vector<std::string> campos;
    std::size_t linha;
};

bool validarUtf8(const std::string& texto) {
    std::size_t i = 0;
    while (i < texto.size()) {
        const unsigned char primeiro = static_cast<unsigned char>(texto[i]);
        if (primeiro <= 0x7F) {
            ++i;
            continue;
        }

        if (primeiro >= 0xC2 && primeiro <= 0xDF) {
            if (i + 1 >= texto.size()) return false;
            const unsigned char segundo = static_cast<unsigned char>(texto[i + 1]);
            if (segundo < 0x80 || segundo > 0xBF) return false;
            i += 2;
            continue;
        }

        if (primeiro >= 0xE0 && primeiro <= 0xEF) {
            if (i + 2 >= texto.size()) return false;
            const unsigned char segundo = static_cast<unsigned char>(texto[i + 1]);
            const unsigned char terceiro = static_cast<unsigned char>(texto[i + 2]);
            if (terceiro < 0x80 || terceiro > 0xBF) return false;
            if (primeiro == 0xE0) {
                if (segundo < 0xA0 || segundo > 0xBF) return false;
            } else if (primeiro == 0xED) {
                if (segundo < 0x80 || segundo > 0x9F) return false;
            } else if (segundo < 0x80 || segundo > 0xBF) {
                return false;
            }
            i += 3;
            continue;
        }

        if (primeiro >= 0xF0 && primeiro <= 0xF4) {
            if (i + 3 >= texto.size()) return false;
            const unsigned char segundo = static_cast<unsigned char>(texto[i + 1]);
            const unsigned char terceiro = static_cast<unsigned char>(texto[i + 2]);
            const unsigned char quarto = static_cast<unsigned char>(texto[i + 3]);
            if (terceiro < 0x80 || terceiro > 0xBF ||
                quarto < 0x80 || quarto > 0xBF) return false;
            if (primeiro == 0xF0) {
                if (segundo < 0x90 || segundo > 0xBF) return false;
            } else if (primeiro == 0xF4) {
                if (segundo < 0x80 || segundo > 0x8F) return false;
            } else if (segundo < 0x80 || segundo > 0xBF) {
                return false;
            }
            i += 4;
            continue;
        }

        return false;
    }
    return true;
}

std::string escaparCampoCsv(const std::string& campo) {
    const bool precisaAspas = campo.find_first_of(",\"\r\n") != std::string::npos;
    if (!precisaAspas) return campo;

    std::string escapado = "\"";
    for (char caractere : campo) {
        if (caractere == '"') escapado += '"';
        escapado += caractere;
    }
    escapado += '"';
    return escapado;
}

bool analisarCsv(const std::string& texto,
                 std::vector<RegistroCsv>& registros,
                 std::size_t& linhaErro,
                 std::string& motivo) {
    std::vector<std::string> campos;
    std::string campo;
    bool dentroDeAspas = false;
    bool aspasFechadas = false;
    bool registroIniciado = false;
    std::size_t linha = 1;
    std::size_t inicioRegistro = 1;

    const auto finalizarCampo = [&]() {
        campos.push_back(campo);
        campo.clear();
        aspasFechadas = false;
        registroIniciado = true;
    };
    const auto finalizarRegistro = [&]() {
        finalizarCampo();
        registros.push_back({campos, inicioRegistro});
        campos.clear();
        registroIniciado = false;
        inicioRegistro = linha + 1;
    };

    for (std::size_t i = 0; i < texto.size(); ++i) {
        const char caractere = texto[i];

        if (dentroDeAspas) {
            if (caractere == '"') {
                if (i + 1 < texto.size() && texto[i + 1] == '"') {
                    campo += '"';
                    ++i;
                } else {
                    dentroDeAspas = false;
                    aspasFechadas = true;
                }
            } else {
                campo += caractere;
                if (caractere == '\r') {
                    if (i + 1 < texto.size() && texto[i + 1] == '\n') {
                        campo += '\n';
                        ++i;
                    }
                    ++linha;
                } else if (caractere == '\n') {
                    ++linha;
                }
            }
            continue;
        }

        if (aspasFechadas) {
            if (caractere == ',') {
                finalizarCampo();
                continue;
            }
            if (caractere == '\r' || caractere == '\n') {
                finalizarRegistro();
                if (caractere == '\r' && i + 1 < texto.size() && texto[i + 1] == '\n') {
                    ++i;
                }
                ++linha;
                inicioRegistro = linha;
                continue;
            }
            linhaErro = linha;
            motivo = "caractere inesperado depois de fechar aspas";
            return false;
        }

        if (caractere == '"') {
            if (!campo.empty()) {
                linhaErro = linha;
                motivo = "aspas so podem iniciar um campo";
                return false;
            }
            dentroDeAspas = true;
            registroIniciado = true;
        } else if (caractere == ',') {
            finalizarCampo();
        } else if (caractere == '\r' || caractere == '\n') {
            finalizarRegistro();
            if (caractere == '\r' && i + 1 < texto.size() && texto[i + 1] == '\n') {
                ++i;
            }
            ++linha;
            inicioRegistro = linha;
        } else {
            campo += caractere;
            registroIniciado = true;
        }
    }

    if (dentroDeAspas) {
        linhaErro = inicioRegistro;
        motivo = "campo entre aspas nao foi fechado";
        return false;
    }

    if (registroIniciado || !campos.empty() || !campo.empty() || aspasFechadas) {
        registros.push_back({campos, inicioRegistro});
        registros.back().campos.push_back(campo);
    }
    return true;
}

template <typename Inteiro>
bool converterInteiro(const std::string& texto, Inteiro& valor) {
    if (texto.empty()) return false;
    Inteiro convertido{};
    const char* inicio = texto.data();
    const char* fim = inicio + texto.size();
    const auto resultado = std::from_chars(inicio, fim, convertido);
    if (resultado.ec != std::errc{} || resultado.ptr != fim) return false;
    valor = convertido;
    return true;
}

std::string criarSufixoTemporario() {
    static std::atomic<unsigned long long> contador{0};
    const auto tempo = std::chrono::steady_clock::now().time_since_epoch().count();
    return "." + std::to_string(GetCurrentProcessId()) + "." +
           std::to_string(tempo) + "." + std::to_string(contador++);
}

void removerTemporario(const std::filesystem::path& caminho) {
    std::error_code ignorado;
    std::filesystem::remove(caminho, ignorado);
}

}  // namespace

bool Persistencia::salvar(const std::string& caminho,
                          const Estoque& estoque,
                          std::string& erro) {
    erro.clear();
    if (caminho.empty()) {
        erro = "caminho do arquivo vazio";
        return false;
    }

    const std::filesystem::path destino(caminho);
    const std::filesystem::path diretorio = destino.parent_path();
    std::error_code codigoErro;
    if (!diretorio.empty()) {
        std::filesystem::create_directories(diretorio, codigoErro);
        if (codigoErro) {
            erro = "nao foi possivel criar o diretorio: " + codigoErro.message();
            return false;
        }
    }

    const std::string sufixo = criarSufixoTemporario();
    std::filesystem::path temporario = destino;
    temporario += ".tmp" + sufixo;
    std::filesystem::path backup = destino;
    backup += ".bak" + sufixo;

    std::ofstream arquivo(temporario, std::ios::binary | std::ios::out | std::ios::trunc);
    if (!arquivo.is_open()) {
        erro = "nao foi possivel abrir o arquivo temporario para escrita";
        return false;
    }

    arquivo << "codigo,nome,precoCentavos,quantidade\n";
    std::size_t numeroProduto = 0;
    for (const Produto& produto : estoque.obterProdutos()) {
        ++numeroProduto;
        if (!validarProduto(produto)) {
            erro = "produto invalido no estoque, posicao " + std::to_string(numeroProduto);
            arquivo.close();
            removerTemporario(temporario);
            return false;
        }
        if (!validarUtf8(produto.codigo) || !validarUtf8(produto.nome)) {
            erro = "codigo ou nome nao esta em UTF-8, posicao " +
                   std::to_string(numeroProduto);
            arquivo.close();
            removerTemporario(temporario);
            return false;
        }

        arquivo << escaparCampoCsv(produto.codigo) << ','
                << escaparCampoCsv(produto.nome) << ','
                << produto.precoCentavos << ',' << produto.quantidade << '\n';
        if (!arquivo) {
            erro = "falha durante a escrita do arquivo temporario";
            arquivo.close();
            removerTemporario(temporario);
            return false;
        }
    }

    arquivo.flush();
    if (!arquivo) {
        erro = "falha ao descarregar os dados no arquivo temporario";
        arquivo.close();
        removerTemporario(temporario);
        return false;
    }
    arquivo.close();
    if (arquivo.fail()) {
        erro = "falha ao fechar o arquivo temporario";
        removerTemporario(temporario);
        return false;
    }

    const bool destinoExiste = std::filesystem::exists(destino, codigoErro);
    if (codigoErro) {
        erro = "nao foi possivel verificar o arquivo atual: " + codigoErro.message();
        removerTemporario(temporario);
        return false;
    }

    if (destinoExiste) {
        if (!ReplaceFileW(destino.c_str(), temporario.c_str(), backup.c_str(),
                          REPLACEFILE_WRITE_THROUGH, nullptr, nullptr)) {
            const DWORD erroWindows = GetLastError();
            std::error_code verificacaoErro;
            const bool destinoAindaExiste = std::filesystem::exists(destino, verificacaoErro);
            const bool backupExiste = std::filesystem::exists(backup, verificacaoErro);
            std::string detalhe = "falha do Windows ao substituir o arquivo (codigo " +
                                 std::to_string(erroWindows) + ")";
            if (!destinoAindaExiste && backupExiste) {
                if (MoveFileExW(backup.c_str(), destino.c_str(), MOVEFILE_WRITE_THROUGH)) {
                    detalhe += "; arquivo original restaurado";
                } else {
                    detalhe += "; restauracao falhou, original preservado em " +
                               backup.string();
                }
            }
            erro = detalhe;
            removerTemporario(temporario);
            return false;
        }

        std::error_code limpezaErro;
        std::filesystem::remove(backup, limpezaErro);
        if (limpezaErro) {
            erro = "dados salvos, mas o backup temporario nao foi removido: " +
                   backup.string();
        }
        return true;
    }

    if (!MoveFileExW(temporario.c_str(), destino.c_str(), MOVEFILE_WRITE_THROUGH)) {
        erro = "falha do Windows ao criar o arquivo de dados (codigo " +
               std::to_string(GetLastError()) + ")";
        removerTemporario(temporario);
        return false;
    }
    return true;
}

bool Persistencia::carregar(const std::string& caminho,
                            Estoque& estoque,
                            std::string& erro) {
    erro.clear();
    const std::filesystem::path caminhoArquivo(caminho);
    std::ifstream arquivo(caminhoArquivo, std::ios::binary);
    if (!arquivo.is_open()) {
        std::error_code codigoErro;
        const bool existe = std::filesystem::exists(caminhoArquivo, codigoErro);
        if (codigoErro) {
            erro = "nao foi possivel verificar o arquivo: " + codigoErro.message();
            return false;
        }
        if (!existe) {
            estoque = Estoque{};
            return true;
        }
        erro = "arquivo existe, mas nao pode ser aberto para leitura";
        return false;
    }

    const std::string cabecalho = "codigo,nome,precoCentavos,quantidade";
    std::string conteudo((std::istreambuf_iterator<char>(arquivo)),
                         std::istreambuf_iterator<char>());
    if (arquivo.bad()) {
        erro = "falha durante a leitura do arquivo";
        return false;
    }

    if (conteudo.size() >= 3 &&
        static_cast<unsigned char>(conteudo[0]) == 0xEF &&
        static_cast<unsigned char>(conteudo[1]) == 0xBB &&
        static_cast<unsigned char>(conteudo[2]) == 0xBF) {
        conteudo.erase(0, 3);
    }

    Estoque estoqueCarregado;
    if (conteudo.empty()) {
        estoque = std::move(estoqueCarregado);
        return true;
    }

    std::vector<RegistroCsv> registros;
    std::size_t linhaErro = 1;
    std::string motivo;
    if (!analisarCsv(conteudo, registros, linhaErro, motivo)) {
        erro = "linha " + std::to_string(linhaErro) + ": " + motivo;
        return false;
    }
    if (registros.empty() || registros.front().campos.size() != 4 ||
        registros.front().campos[0] != "codigo" ||
        registros.front().campos[1] != "nome" ||
        registros.front().campos[2] != "precoCentavos" ||
        registros.front().campos[3] != "quantidade") {
        erro = "linha 1: cabecalho CSV incorreto";
        return false;
    }

    for (std::size_t i = 1; i < registros.size(); ++i) {
        const RegistroCsv& registro = registros[i];
        if (registro.campos.size() != 4) {
            erro = "linha " + std::to_string(registro.linha) +
                   ": registro deve conter exatamente quatro campos";
            return false;
        }

        long long precoCentavos = 0;
        int quantidade = 0;
        if (!converterInteiro(registro.campos[2], precoCentavos) ||
            !converterInteiro(registro.campos[3], quantidade)) {
            erro = "linha " + std::to_string(registro.linha) +
                   ": preco ou quantidade invalido ou fora do limite";
            return false;
        }
        if (!validarUtf8(registro.campos[0]) || !validarUtf8(registro.campos[1])) {
            erro = "linha " + std::to_string(registro.linha) +
                   ": codigo ou nome contem UTF-8 invalido";
            return false;
        }

        const Produto produto{registro.campos[0], registro.campos[1],
                              precoCentavos, quantidade};
        if (!validarProduto(produto)) {
            erro = "linha " + std::to_string(registro.linha) +
                   ": dados do produto invalidos";
            return false;
        }
        if (!estoqueCarregado.cadastrarProduto(produto)) {
            erro = "linha " + std::to_string(registro.linha) +
                   ": codigo duplicado ou produto rejeitado pelo estoque";
            return false;
        }
    }

    estoque = std::move(estoqueCarregado);
    return true;
}

# StockManager

Sistema de gerenciamento de estoque desenvolvido em C++17, com execução pelo terminal.

O projeto permite cadastrar produtos, controlar entradas e saídas e acompanhar a situação do estoque.

## Funcionalidades

- Cadastro, consulta, edição e exclusão de produtos.
- Controle de entrada e saída de mercadorias.
- Consulta ao histórico de movimentações.
- Alertas de estoque baixo e produtos sem unidades.
- Relatórios do estoque e das movimentações.
- Salvamento automático dos dados em arquivos CSV.

## Tecnologias

- C++17
- Arquivos CSV para armazenamento dos dados.
- PowerShell para compilação e execução.

## Como executar

**Requisitos:**
- Windows 10 ou superior.
- MSYS2 com compilador G++ compatível com C++17.

Na pasta do projeto, execute:

```powershell
& 'C:\msys64\ucrt64\bin\g++.exe' -std=c++17 -Wall -Wextra -pedantic `
  -Iinclude src/main.cpp src/Produto.cpp src/Estoque.cpp src/Entrada.cpp `
  src/Persistencia.cpp src/Historico.cpp src/Relatorios.cpp `
  -o StockManager.exe

./StockManager.exe
```

## Testes

Para executar os testes automatizados:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tests\run_tests.ps1
```

## Estrutura do projeto

- `include/`: arquivos de cabeçalho.
- `src/`: código-fonte do sistema.
- `tests/`: testes automatizados.
- `data/`: arquivos utilizados para armazenar os dados do estoque.

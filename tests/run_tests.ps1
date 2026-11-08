$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$compiler = 'C:\msys64\ucrt64\bin\g++.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    throw "G++ nao encontrado em $compiler"
}

$temporaryRoot = Join-Path ([System.IO.Path]::GetTempPath()) (
    'stockmanager-tests-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temporaryRoot | Out-Null

function Assert-Contains([string] $text, [string] $expected, [string] $context) {
    if (-not $text.Contains($expected)) {
        throw "Falha em '$context': texto esperado nao encontrado: $expected`n$text"
    }
}

function Invoke-StockManager([string] $executable, [string] $workingDirectory,
                             [string] $inputText) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = $executable
    $startInfo.WorkingDirectory = $workingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    if (-not $process.Start()) { throw 'Nao foi possivel iniciar o StockManager.' }
    $process.StandardInput.Write($inputText)
    $process.StandardInput.Close()
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    if (-not $process.WaitForExit(15000)) {
        $process.Kill()
        throw 'O StockManager excedeu o tempo limite do teste.'
    }
    if ($process.ExitCode -ne 0) {
        throw "StockManager terminou com codigo $($process.ExitCode).`n$stderr`n$stdout"
    }
    return $stdout + $stderr
}

try {
    $sources = @(
        'src/Produto.cpp', 'src/Estoque.cpp', 'src/Entrada.cpp',
        'src/Persistencia.cpp', 'src/Historico.cpp', 'src/Relatorios.cpp'
    ) | ForEach-Object { Join-Path $projectRoot $_ }
    $testExecutable = Join-Path $temporaryRoot 'test_stockmanager.exe'
    $appExecutable = Join-Path $temporaryRoot 'StockManager.exe'

    Push-Location $projectRoot
    try {
        & $compiler -std=c++17 -Wall -Wextra -pedantic -Iinclude `
            (Join-Path $projectRoot 'tests/test_stockmanager.cpp') @sources `
            -o $testExecutable
        if ($LASTEXITCODE -ne 0) { throw "Compilacao da suite falhou ($LASTEXITCODE)." }

        & $compiler -std=c++17 -Wall -Wextra -pedantic -Iinclude `
            (Join-Path $projectRoot 'src/main.cpp') @sources -o $appExecutable
        if ($LASTEXITCODE -ne 0) { throw "Compilacao do programa falhou ($LASTEXITCODE)." }
    }
    finally { Pop-Location }

    $unitData = Join-Path $temporaryRoot 'dados-unitarios'
    & $testExecutable $unitData
    if ($LASTEXITCODE -ne 0) { throw "Testes de dominio/persistencia falharam ($LASTEXITCODE)." }
    Write-Output 'OK: testes de dominio, CSV, integridade e falhas de arquivo.'

    $integration = Join-Path $temporaryRoot 'integracao'
    New-Item -ItemType Directory -Path $integration | Out-Null
    $inputScript = @(
        '1', 'K1', 'Teclado', '10,50', '10',
        '4', 'K1', '5',
        '5', 'K1', '5',
        '6', 'K1', 'K1-NOVO', 'Teclado Gamer', '12,00', '12',
        '7', 'K1-NOVO', 'n',
        '8', '9', '10',
        '7', 'K1-NOVO', 'S',
        '10', '0'
    ) -join "`n"
    $output = Invoke-StockManager $appExecutable $integration ($inputScript + "`n")
    Assert-Contains $output 'Produto cadastrado e salvo com sucesso.' 'cadastro terminal'
    Assert-Contains $output 'Entrada registrada e salva com sucesso.' 'entrada terminal'
    Assert-Contains $output 'Saida registrada e salva com sucesso.' 'saida terminal'
    Assert-Contains $output 'Produto atualizado e salvo.' 'edicao terminal'
    Assert-Contains $output 'Exclusao cancelada.' 'cancelamento de exclusao'
    Assert-Contains $output 'Teclado Gamer' 'listagem apos edicao'
    Assert-Contains $output '10 unidades' 'quantidade preservada na edicao'
    Assert-Contains $output 'Abaixo do minimo: 1' 'alerta de estoque minimo'
    Assert-Contains $output 'Unidades de entrada: 5' 'relatorio de entrada'
    Assert-Contains $output 'Unidades de saida: 5' 'relatorio de saida'
    Assert-Contains $output 'Produto excluido.' 'exclusao confirmada'
    if (-not (Test-Path (Join-Path $integration 'data/produtos.csv'))) {
        throw 'Arquivo persistido de produtos nao foi criado.'
    }
    if (-not (Test-Path (Join-Path $integration 'data/movimentacoes.csv'))) {
        throw 'Arquivo persistido de historico nao foi criado.'
    }
    Write-Output 'OK: fluxo de terminal, minimo, relatorios, edicao/exclusao e persistencia.'

    $restart = Invoke-StockManager $appExecutable $integration "3`n10`n0`n"
    Assert-Contains $restart 'Estoque vazio.' 'persistencia apos excluir produto'
    Assert-Contains $restart 'ENTRADA | K1 | Teclado | 10 -> 15' 'historico preservado'
    Assert-Contains $restart 'SAIDA | K1 | Teclado | 15 -> 10' 'historico apos reiniciar'
    Write-Output 'OK: recarga apos reiniciar; historico mantido mesmo com produto excluido.'

    $failureDirectory = Join-Path $temporaryRoot 'falha-de-gravacao'
    $failureData = Join-Path $failureDirectory 'data'
    New-Item -ItemType Directory -Path $failureData -Force | Out-Null
    $productsFile = Join-Path $failureData 'produtos.csv'
    [System.IO.File]::WriteAllText(
        $productsFile,
        "codigo,nome,precoCentavos,quantidade,quantidadeMinima`n",
        [System.Text.Encoding]::UTF8)
    $fileLock = [System.IO.File]::Open(
        $productsFile,
        [System.IO.FileMode]::Open,
        [System.IO.FileAccess]::Read,
        [System.IO.FileShare]::ReadWrite)
    try {
        $failedSave = Invoke-StockManager $appExecutable $failureDirectory `
            ("1`nLOCK`nItem`n1,00`n2`n3`n0`n")
    }
    finally { $fileLock.Dispose() }
    Assert-Contains $failedSave 'Cadastro cancelado; estoque anterior restaurado.' `
        'rollback de cadastro apos falha de persistencia'
    Assert-Contains $failedSave 'Estoque vazio.' 'estoque em memoria revertido'
    if ($failedSave.Contains('Produto cadastrado e salvo com sucesso.')) {
        throw 'Uma gravacao recusada foi apresentada como sucesso.'
    }
    Write-Output 'OK: falha de substituicao recusa sucesso e reverte o estado em memoria.'
}
finally {
    if (Test-Path -LiteralPath $temporaryRoot) {
        Remove-Item -LiteralPath $temporaryRoot -Recurse -Force
    }
}

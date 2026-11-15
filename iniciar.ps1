$ErrorActionPreference = "Stop"
$env:PATH = "C:\msys64\ucrt64\bin;C:\Program Files\Git\cmd;" + $env:PATH
Set-Location $PSScriptRoot
if (-not (Test-Path "build\CMakeCache.txt")) {
    cmake -S . -B build -G Ninja '-DGIT_EXECUTABLE=C:/Program Files/Git/cmd/git.exe' -DFETCHCONTENT_UPDATES_DISCONNECTED=ON
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
cmake --build build --target simulador --parallel 6
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& "$PSScriptRoot\build\simulador-emergencia-enclavada.exe"

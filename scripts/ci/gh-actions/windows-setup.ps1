$ErrorActionPreference = "Stop"

Write-Host "::group::Setup CONDA"
$Env:Path += ";$Env:CONDA\condabin"
conda.bat init powershell
conda.bat init bash
Write-Host "::endgroup::"

Write-Host "::group::Installing common deps"
# We cannot use conda channels due to licenses, use conda-forge
conda.bat config --prepend channels conda-forge
conda.bat config --append channels nodefaults
conda.bat config --remove channels defaults
conda.bat config --system --remove channels defaults
conda.bat config --show channels
conda.bat config --set channel_priority strict
conda.bat env create -f "gha\scripts\ci\gh-actions\conda-env-win.yml"
Write-Host "::endgroup::"

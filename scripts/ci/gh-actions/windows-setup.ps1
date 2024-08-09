$ErrorActionPreference = "Stop"

Write-Host "::group::Setup CONDA"
$Env:Path += ";$Env:CONDA\condabin"
conda.bat init powershell
conda.bat init bash
conda.bat update --all -y
conda.bat config --add channels conda-forge
conda.bat config --add channels msys2
conda.bat config --set channel_priority strict
Write-Host "::endgroup::"

Write-Host "::group::Installing common deps"
conda.bat env create -f "gha\scripts\ci\gh-actions\conda-env-win.yml"
Write-Host "::endgroup::"

if($Env:GH_YML_MATRIX_PARALLEL -eq "msmpi")
{
  Write-Host "::group::Installing msmpi"
  conda.bat install -n adios2 -c conda-forge -y "msmpi" "mpi4py"
  Write-Host "::endgroup::"
}

conda.bat list -n adios2
conda.bat info --verbose

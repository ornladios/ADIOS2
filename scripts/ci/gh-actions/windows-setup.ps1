$ErrorActionPreference = "Stop"

Write-Host "::group::Setup CONDA"
$Env:Path += ";$Env:CONDA\condabin"
conda.bat init powershell
conda.bat init bash
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

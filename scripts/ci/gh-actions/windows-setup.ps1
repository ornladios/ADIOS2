$ErrorActionPreference = "Stop"

Write-Host "::group::Updating pip"
python3 -m pip install --upgrade pip
Write-Host "::endgroup::"

Write-Host "::group::Installing NumPy"
pip install "numpy>=1.19"
Write-Host "::endgroup::"

if($Env:GH_YML_MATRIX_PARALLEL -eq "mpi")
{
  $rooturl = "https://github.com/microsoft/Microsoft-MPI/releases/download"
    $version = "10.1.1"
    $baseurl = "$rooturl/v$version"

    $tempdir    = $Env:RUNNER_TEMP
    $msmpisdk   = Join-Path $tempdir msmpisdk.msi
    $msmpisetup = Join-Path $tempdir msmpisetup.exe

    Write-Host "::group::Downloading Microsoft MPI SDK $version"
    Invoke-WebRequest "$baseurl/msmpisdk.msi"   -OutFile $msmpisdk
    Write-Host "::endgroup::"
    Write-Host "::group::Installing Microsoft MPI SDK $version"
    Start-Process msiexec.exe -ArgumentList "/quiet /passive /qn /i $msmpisdk" -Wait
    Write-Host "::endgroup::"

    Write-Host "::group::Downloading Microsoft MPI Runtime $version"
    Invoke-WebRequest "$baseurl/msmpisetup.exe" -OutFile $msmpisetup
    Write-Host "::endgroup::"
    Write-Host "::group::Installing Microsoft MPI Runtime $version"
    Start-Process $msmpisetup -ArgumentList "-unattend" -Wait
    Write-Host "::endgroup::"

    if ($Env:GITHUB_ENV) {
      Write-Host '::group::Adding environment variables to $GITHUB_ENV'
        $envlist = @("MSMPI_BIN", "MSMPI_INC", "MSMPI_LIB32", "MSMPI_LIB64")
        foreach ($name in $envlist) {
          $value = [Environment]::GetEnvironmentVariable($name, "Machine")
            Write-Host "$name=$value"
            Add-Content $Env:GITHUB_ENV "$name=$value"
        }
      Write-Host "::endgroup::"
    }

  if ($Env:GITHUB_PATH) {
    Write-Host '::group::Adding $MSMPI_BIN to $GITHUB_PATH'
      $MSMPI_BIN = [Environment]::GetEnvironmentVariable("MSMPI_BIN", "Machine")
      Add-Content $Env:GITHUB_PATH $MSMPI_BIN
      Write-Host "::endgroup::"
  }

  Write-Host "::group::Installing mpi4py"
  pip install "mpi4py>=1.03"
  Write-Host "::endgroup::"
}


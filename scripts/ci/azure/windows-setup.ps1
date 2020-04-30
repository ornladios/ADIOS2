$ErrorActionPreference = "Stop"
$BaseDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
$WebClient = New-Object System.Net.WebClient

# The visual studio import uses the Invoke-CmdScript from Wintellect:
# https://github.com/Wintellect/WintellectPowerShell/blob/master/Code/Internal.ps1
. ("$BaseDir\windows-import-visualstudioenvironment.ps1")

################################################################################

function main () {
    $jobName = $(Get-Item -Path "Env:SYSTEM_JOBNAME").Value
    Write-Host "##vso[task.setvariable variable=CC;]cl"
    Write-Host "##vso[task.setvariable variable=CXX;]cl"

    # Setup the MSVC environment
    Write-Host "Setting up Visual Studio environment"
    if($jobName -match "vs(20[0-9][0-9]).*") {
        Import-VisualStudioEnvironment $($matches[1])
        $envlist = @("PATH", "INCLUDE", "LIB", "LIBPATH")
        foreach ($variable in $envlist) {
            $value = $(Get-Item -Path Env:$variable).Value
            if ($value) {
                Write-Host "##vso[task.setvariable variable=$variable;]$value"
            }
        }
    }

    # Install CMake 3.17.2
    Write-Host "Downloading CMake"
    $WebClient.DownloadFile("https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-win64-x64.msi", "cmake-3.17.2-win64-x64.msi")
    Write-Host "Installing CMake"
    Start-Process -FilePath "msiexec.exe" -ArgumentList "/qb","/i","cmake-3.17.2-win64-x64.msi","ADD_CMAKE_TO_PATH=System" -Wait

    # Install Ninja
    if($jobName -match "ninja") {
        Write-Host "Installing Ninja"
        choco install -y ninja
    }

    # Install MS-MPI
    if($jobName -match "msmpi") {
        Write-Host "Downloading MS-MPI Runtime"
        $WebClient.DownloadFile("https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisetup.exe", "msmpisetup.exe")
        Write-Host "Installing MS-MPI Runtime"
        Start-Process "msmpisetup.exe" -ArgumentList "-unattend" -Wait -NoNewWindow

        Write-Host "Downloading MS-MPI SDK"
        $WebClient.DownloadFile("https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisdk.msi", "msmpisdk.msi")
        Write-Host "Installing MS-MPI SDK"
        Start-Process "msiexec.exe" -ArgumentList "/qb","/i","msmpisdk.msi" -Wait

        Write-Host "##vso[task.setvariable variable=MSMPI_BIN;]C:\Program Files\Microsoft MPI\Bin"
        Write-Host "##vso[task.setvariable variable=MSMPI_INC;]C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
        Write-Host "##vso[task.setvariable variable=MSMPI_LIB32;]C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86"
        Write-Host "##vso[task.setvariable variable=MSMPI_LIB64;]C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"
    }

    # Install Numpy
    #Write-Host "Installing NumPy"
    #choco install -y numpy

    # Install mpi4py
    #if($jobName -match "msmpi") {
    #    Write-Host "Installing mpi4py"
    #    pip install mpi4py
    #}
}

main


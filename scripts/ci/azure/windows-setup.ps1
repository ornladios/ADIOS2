$ErrorActionPreference = "Stop"
$BaseDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

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

    # Install CMake nightly
    Install-MSI -MsiUrl "https://cmake.org/files/dev/cmake-3.16.20191218-g8262562-win64-x64.msi" -MsiName "cmake-3.16.20191218-g8262562-win64-x64.msi" -ArgumentList "ADD_CMAKE_TO_PATH=System"

    # Install Ninja
    if($jobName -match "ninja") {
        Write-Host "Installing Ninja"
        choco install -y ninja
    }

    # Install numpy
    pip3 install numpy

    # Install MS-MPI
    if($jobName -match "msmpi") {
        Write-Host "Installing MS-MPI"
        Install-EXE -Url "https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisetup.exe"  -Name "msmpisetup.exe" -ArgumentList "-unattend"
        Install-MSI -MsiUrl "https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisdk.msi" -MsiName "msmpisdk.msi"
        Write-Host "##vso[task.setvariable variable=MSMPI_BIN;]C:\Program Files\Microsoft MPI\Bin"
        Write-Host "##vso[task.setvariable variable=MSMPI_INC;]C:\Program Files (x86)\Microsoft SDKs\MPI\Include"
        Write-Host "##vso[task.setvariable variable=MSMPI_LIB32;]C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86"
        Write-Host "##vso[task.setvariable variable=MSMPI_LIB64;]C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"

        pip3 install mpi4py
    }
}

main


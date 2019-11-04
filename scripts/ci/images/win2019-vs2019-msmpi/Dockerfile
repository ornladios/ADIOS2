#FROM mcr.microsoft.com/windows/servercore:ltsc2019
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019

# Change the default shell to PowerShell
SHELL ["powershell", "-NoLogo", "-NoProfile", "-Command"]

# Install Chocolatey
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1')) ; \
    choco upgrade all -y

# Install base libraries and tools
RUN choco install -y choco-cleaner git bzip2 ninja

# Install CMake
RUN choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System'

# Install Python
RUN choco install -y python numpy

# Install MS-MPI
RUN (New-Object System.Net.WebClient).DownloadFile('http://download.microsoft.com/download/2/9/e/29efe9b1-16d7-4912-a229-6734b0c4e235/msmpisdk.msi', '\Windows\Temp\msmpisdk.msi') ; \
    msiexec /quiet /qn /i \Windows\Temp\msmpisdk.msi
RUN (New-Object System.Net.WebClient).DownloadFile('http://download.microsoft.com/download/2/9/e/29efe9b1-16d7-4912-a229-6734b0c4e235/MSMpiSetup.exe', '\Windows\Temp\MSMpiSetup.exe') ; \
    \Windows\Temp\MSMpiSetup.exe -unattend
ENV MSMPI_BIN="C:\Program Files\Microsoft MPI\Bin" \
    MSMPI_INC="C:\Program Files (x86)\Microsoft SDKs\MPI\Include" \
    MSMPI_LIB32="C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x86" \
    MSMPI_LIB64="C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"

# PATH isn't actually set in the Docker image, so we have to set it from 
# within the container
RUN $newPath = ('{0};{1}' -f $env:MSMPI_BIN, $env:PATH); \
    Write-Host ('Updating PATH: {0}' -f $newPath); \
    [Environment]::SetEnvironmentVariable('PATH', $newPath, [EnvironmentVariableTarget]::Machine);

# Install Visual Studio
RUN choco install -y visualstudio2019-workload-vctools

# Cleanup
RUN Set-ExecutionPolicy -ExecutionPolicy Unrestricted ; \
    \ProgramData\chocolatey\bin\Choco-Cleaner.ps1 ; \
    Remove-Item \Windows\Temp\msmpisdk.msi ; \
    Remove-Item \Windows\Temp\MSMpiSetup.exe ; 

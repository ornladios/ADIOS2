#FROM mcr.microsoft.com/windows/servercore:ltsc2019
FROM mcr.microsoft.com/dotnet/framework/sdk:4.8-windowsservercore-ltsc2019

# Change the default shell to PowerShell
SHELL ["powershell", "-NoLogo", "-NoProfile", "-Command"]

# 1.  Install MS-MPI
ADD https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisetup.exe /Windows/Temp/
ADD https://github.com/microsoft/Microsoft-MPI/releases/download/v10.1.1/msmpisdk.msi /Windows/Temp/
RUN Write-Host ('Installing MPI Runtime') ; \
    Start-Process 'C:\Windows\Temp\msmpisetup.exe' -Wait -ArgumentList '-unattend' ; \
    Remove-Item 'C:\Windows\Temp\msmpisetup.exe' ; \
    Write-Host ('Installing MPI SDK') ; \
    Start-Process msiexec.exe -Wait -ArgumentList '/i C:\Windows\Temp\msmpisdk.msi /quiet' ; \
    Remove-Item /Windows/Temp/msmpisdk.msi

# 2.  Install base libraries and tools through chocolatey
#         (git, ninja, cmake, python numpy, etc.)
RUN Set-ExecutionPolicy Bypass -Scope Process -Force; \
    iex ((New-Object System.Net.WebClient).DownloadString('https://chocolatey.org/install.ps1')) ; \
    choco install -y ninja python numpy ; \
    choco install -y git -params '"/GitAndUnixToolsOnPath"' ; \
    choco install -y cmake --installargs 'ADD_CMAKE_TO_PATH=System' ; \
    choco install -y choco-cleaner ; \
    refreshenv ; \
    /ProgramData/chocolatey/bin/Choco-Cleaner.ps1

# 3.  Install Visual Studio
RUN choco install -y \
        visualstudio2017buildtools \
        visualstudio2017-workload-vctools ; \
    /ProgramData/chocolatey/bin/Choco-Cleaner.ps1

# 4.  Install mpi4py
RUN pip install mpi4py

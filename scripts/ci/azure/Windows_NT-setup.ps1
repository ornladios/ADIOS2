$ErrorActionPreference = "Stop"
$BaseDir = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent

# The visual studio import uses the Invoke-CmdScript from Wintellect:
# https://github.com/Wintellect/WintellectPowerShell/blob/master/Code/Internal.ps1
. ("$BaseDir\windows-import-visualstudioenvironment.ps1")

################################################################################

function main () {
    $jobName = $(Get-Item -Path "Env:SYSTEM_JOBNAME").Value

    # Setup the environment beforehand
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
}

main

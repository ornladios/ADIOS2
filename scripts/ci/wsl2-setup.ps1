Set-PSDebug -Trace 1

choco install -y Microsoft-Windows-Subsystem-Linux --source="'windowsfeatures'"
choco install -y wsl-ubuntu-1804
RefreshEnv

wsl -l -v
wsl --set-version Ubuntu
wsl --set-default-version 2

Param(
    [Parameter(Mandatory=$true)]
    [string]$processName,
    [Parameter(Mandatory=$true)]
    [string[]]$commandLine,
    [string]$windowTitle=$null
)

Write-Host "Starting $processName monitor"
do {
    if ($windowTitle)
    {
        $p = Get-Process $processName -ErrorAction SilentlyContinue | where { $_.MainWindowTitle -eq $windowTitle} | Select-Object -First 1
    }
    else
    {
        $p = Get-Process $processName -ErrorAction SilentlyContinue | Select-Object -First 1
    }

    if ($p) {
        $p.WaitForExit()
    }

    Write-Host "Starting $processName"
    if ($commandLine[1..10])
    {
        Start-Process -FilePath $commandLine[0] -ArgumentList $commandLine[1..10]
    }
    else
    {
        Start-Process -FilePath $commandLine[0]
    }

    Start-Sleep 5

} while ($true)
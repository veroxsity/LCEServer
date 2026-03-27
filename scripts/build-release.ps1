$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$serverDir = Split-Path -Parent $scriptDir
$outputDir = Join-Path $scriptDir "output"

$clean = $false
$version = $null
$config = "Release"
$runtime = "win-x64"

for ($index = 0; $index -lt $args.Count; $index++) {
    switch -Regex ($args[$index]) {
        '^--clean$|^-Clean$' {
            $clean = $true
            continue
        }
        '^--version$|^-Version$' {
            if ($index + 1 -ge $args.Count) {
                throw "Missing value for --version"
            }

            $index++
            $version = $args[$index]
            continue
        }
        '^--config$|^-Config$' {
            if ($index + 1 -ge $args.Count) {
                throw "Missing value for --config"
            }

            $index++
            $config = $args[$index]
            continue
        }
        default {
            throw "Unknown argument: $($args[$index])"
        }
    }
}

function Normalize-Version([string]$value) {
    $trimmed = $value.Trim()
    if ($trimmed.StartsWith('v', [System.StringComparison]::OrdinalIgnoreCase)) {
        return "v$($trimmed.Substring(1))"
    }

    if ([string]::IsNullOrWhiteSpace($trimmed)) {
        throw "Version cannot be empty."
    }

    return "v$trimmed"
}

function Remove-IfExists([string]$path) {
    if (Test-Path $path) {
        Remove-Item -Recurse -Force $path
    }
}

$versionLabel = if ($version) { Normalize-Version $version } else { "dev" }
$packageName = "LCEServer-$versionLabel-$runtime"
$stagingDir = Join-Path $outputDir $packageName

if ($clean) {
    Remove-IfExists $stagingDir
}

New-Item -ItemType Directory -Force -Path $outputDir | Out-Null

Push-Location $serverDir
try {
    & cmake --build build --config $config | Out-Host
}
finally {
    Pop-Location
}

$buildOutputDir = Join-Path $serverDir "build\$config"
$exePath = Join-Path $buildOutputDir "LCEServer.exe"

if (-not (Test-Path $exePath)) {
    throw "Server executable not found: $exePath"
}

Remove-IfExists $stagingDir
New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null

Copy-Item -Path $exePath -Destination (Join-Path $stagingDir "LCEServer.exe") -Force

Write-Host ""
Write-Host "Output folder:"
Write-Host "  $stagingDir"
Write-Host "Version:"
Write-Host "  $versionLabel"
Write-Host "Config:"
Write-Host "  $config"

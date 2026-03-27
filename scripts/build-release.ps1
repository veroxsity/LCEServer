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
$zipPath = Join-Path $outputDir "$packageName.zip"

if ($clean) {
    Remove-IfExists $stagingDir
    Remove-IfExists $zipPath
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
$zlibPath = Join-Path $serverDir "build\_deps\zlib-build\$config\zlib.dll"
$readmePath = Join-Path $serverDir "README.md"

if (-not (Test-Path $exePath)) {
    throw "Server executable not found: $exePath"
}

if (-not (Test-Path $zlibPath)) {
    throw "zlib runtime not found: $zlibPath"
}

Remove-IfExists $stagingDir
New-Item -ItemType Directory -Force -Path $stagingDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stagingDir "logs") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stagingDir "plugins") | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $stagingDir "worlds") | Out-Null

Copy-Item -Path $exePath -Destination (Join-Path $stagingDir "LCEServer.exe") -Force
Copy-Item -Path $zlibPath -Destination (Join-Path $stagingDir "zlib.dll") -Force
Copy-Item -Path $readmePath -Destination (Join-Path $stagingDir "README.md") -Force

$serverProperties = @(
    "# Network"
    "server-port=25565"
    "server-ip=0.0.0.0"
    "server-name=LCEServer"
    "lan-advertise=true"
    ""
    "# World"
    "level-name=world"
    "level-id="
    "level-seed="
    "level-type=default"
    "max-build-height=128"
    ""
    "# Game settings"
    "gamemode=0"
    "difficulty=1"
    "pvp=true"
    "trust-players=false"
    "fire-spreads=true"
    "tnt=true"
    "spawn-animals=true"
    "spawn-npcs=true"
    "spawn-monsters=true"
    "allow-flight=true"
    "allow-nether=true"
    "generate-structures=true"
    "bonus-chest=false"
    "friends-of-friends=true"
    "gamertags=true"
    "bedrock-fog=false"
    "mob-griefing=true"
    "keep-inventory=false"
    "natural-regeneration=true"
    "do-daylight-cycle=true"
    ""
    "# Access control"
    "white-list=false"
    ""
    "# Server"
    "max-players=8"
    "view-distance=4"
    "chunks-per-tick=10"
    "motd=A Minecraft LCE Server"
    "autosave-interval=300"
    "log-level=info"
    "disable-saving=false"
) -join [Environment]::NewLine

Set-Content -Path (Join-Path $stagingDir "server.properties") -Value ($serverProperties + [Environment]::NewLine)
Set-Content -Path (Join-Path $stagingDir "banned-ips.json") -Value "[]`n"
Set-Content -Path (Join-Path $stagingDir "banned-players.json") -Value "[]`n"
Set-Content -Path (Join-Path $stagingDir "ops.json") -Value "[]`n"
Set-Content -Path (Join-Path $stagingDir "whitelist.json") -Value "[]`n"

if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}

Compress-Archive -Path (Join-Path $stagingDir "*") -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host ""
Write-Host "Release folder:"
Write-Host "  $stagingDir"
Write-Host "Release zip:"
Write-Host "  $zipPath"
Write-Host "Version:"
Write-Host "  $versionLabel"
Write-Host "Config:"
Write-Host "  $config"

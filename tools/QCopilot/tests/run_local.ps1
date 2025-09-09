param(
  [string]$BuildDir = "build-qcopilot-local",
  [switch]$JUnit,
  [ValidateSet("Debug","Release")]
  [string]$Config = "Release"
)

$ErrorActionPreference = "Stop"

if (!(Test-Path $BuildDir)) { New-Item -ItemType Directory -Path $BuildDir | Out-Null }

# Configure
cmake -S . -B $BuildDir `
  -DLLAMA_BUILD_TESTS=ON `
  -DLLAMA_BUILD_SERVER=ON `
  -DCMAKE_BUILD_TYPE=$Config

# Build
cmake --build $BuildDir --config $Config -m

# Test
Push-Location $BuildDir
New-Item -ItemType Directory -Path test-results -ErrorAction SilentlyContinue | Out-Null

if ($JUnit) {
  ctest --output-on-failure -R test_ -C $Config --output-junit test-results/qcopilot-tests.xml
} else {
  ctest --output-on-failure -R test_ -C $Config
}

Pop-Location
Write-Host "Done. Build dir: $BuildDir"


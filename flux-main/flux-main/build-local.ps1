# Local Windows build of Flux with the bindings pinned to a known-good commit.
# Run from the project folder in PowerShell:   .\build-local.ps1
#
# Why pin: Geode SDK v5.11.2 normally clones geode-sdk/bindings at branch "main"
# via CPM. That branch moves daily and its code generator changed after the SDK
# tag (PredeclareGen / BindingGen), which is the most likely cause of the
# "SequenceTriggerState / EffectManagerState is undefined" errors.

$ErrorActionPreference = "Stop"
$bindingsSha = "1605f500d053f8de122bf5c7dc6d17c24866e02e"
$bindingsDir = Join-Path $PSScriptRoot "bindings-pinned"

if (-not $env:GEODE_SDK) { throw "GEODE_SDK is not set. Install the Geode SDK with the geode CLI first." }

if (-not (Test-Path $bindingsDir)) {
    git clone https://github.com/geode-sdk/bindings $bindingsDir
}
git -C $bindingsDir fetch origin
git -C $bindingsDir checkout $bindingsSha

# SDK must be exactly v5.11.2 for this pin to be consistent.
git -C $env:GEODE_SDK fetch --tags
git -C $env:GEODE_SDK checkout v5.11.2
geode sdk install-binaries --version 5.11.2

$env:GEODE_BINDINGS_REPO_PATH = $bindingsDir

# Fresh build directory: stale generated bindings from earlier attempts must not be reused.
if (Test-Path build) { Remove-Item build -Recurse -Force }
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

Get-ChildItem build -Filter *.geode

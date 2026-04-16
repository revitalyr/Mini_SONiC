# Build script for Mini Switch - Windows PowerShell version
param(
    [switch]$Clean,
    [switch]$Debug
)

Write-Host "Building Mini Switch..." -ForegroundColor Green

# Create directories
New-Item -ItemType Directory -Force -Path obj | Out-Null
New-Item -ItemType Directory -Force -Path bin | Out-Null

if ($Clean) {
    Write-Host "Cleaning build directories..."
    Remove-Item obj\* -Force -ErrorAction SilentlyContinue
    Remove-Item bin\* -Force -ErrorAction SilentlyContinue
}

# Set compiler flags
$CFLAGS = "/I include"
if ($Debug) {
    $CFLAGS += " /Zi /Od /DEBUG"
} else {
    $CFLAGS += " /O2"
}

# Source files
$SOURCES = @(
    "src/main.c",
    "src/port.c", 
    "src/ring_buffer.c",
    "src/ethernet.c",
    "src/mac_table.c",
    "src/vlan.c",
    "src/arp.c",
    "src/ip.c",
    "src/forwarding.c"
)

# Compile source files
Write-Host "Compiling source files..." -ForegroundColor Yellow
$objects = @()

foreach ($source in $SOURCES) {
    $object = "obj\" + [System.IO.Path]::GetFileNameWithoutExtension($source) + ".obj"
    Write-Host "  Compiling $source -> $object"
    
    $cmd = "cl.exe $CFLAGS /c $source /Fo$object"
    Invoke-Expression $cmd
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Compilation failed for $source" -ForegroundColor Red
        exit 1
    }
    
    $objects += $object
}

# Link
Write-Host "Linking..." -ForegroundColor Yellow
$linkCmd = "cl.exe $objects /Fe:bin\mini_switch.exe"
Invoke-Expression $linkCmd

if ($LASTEXITCODE -ne 0) {
    Write-Host "Linking failed" -ForegroundColor Red
    exit 1
}

Write-Host "Build complete! Binary: bin\mini_switch.exe" -ForegroundColor Green

# Check if executable exists
if (Test-Path "bin\mini_switch.exe") {
    $size = (Get-Item "bin\mini_switch.exe").Length
    Write-Host "Executable size: $size bytes" -ForegroundColor Cyan
}

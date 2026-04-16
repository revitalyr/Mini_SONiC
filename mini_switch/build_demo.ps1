# Build and run simple Mini Switch demo
Write-Host "Building Mini Switch Simple Demo..." -ForegroundColor Green

# Create directories
New-Item -ItemType Directory -Force -Path obj | Out-Null
New-Item -ItemType Directory -Force -Path bin | Out-Null

# Compile simple demo (no networking dependencies)
Write-Host "Compiling simple demo..." -ForegroundColor Yellow
$cmd = "cl.exe src\demo_simple.c /Fe:bin\mini_switch_demo.exe"
Write-Host "  Command: $cmd"

try {
    Invoke-Expression $cmd
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "Build successful!" -ForegroundColor Green
        
        if (Test-Path "bin\mini_switch_demo.exe") {
            $size = (Get-Item "bin\mini_switch_demo.exe").Length
            Write-Host "Executable created: bin\mini_switch_demo.exe ($size bytes)" -ForegroundColor Cyan
            
            # Run the demo
            Write-Host "Running Mini Switch demo..." -ForegroundColor Yellow
            Write-Host "================================" -ForegroundColor Blue
            
            & ".\bin\mini_switch_demo.exe"
            
            Write-Host "================================" -ForegroundColor Blue
            Write-Host "Demo completed!" -ForegroundColor Green
        }
    } else {
        Write-Host "Build failed with exit code: $LASTEXITCODE" -ForegroundColor Red
    }
} catch {
    Write-Host "Build failed: $_" -ForegroundColor Red
}

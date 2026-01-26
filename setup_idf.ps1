# ESP-IDF Environment Setup Script
# Run this before building: .\setup_idf.ps1

Write-Host "Setting up ESP-IDF environment..." -ForegroundColor Cyan

# Find ESP-IDF installation
$possible_paths = @(
    "$env:USERPROFILE\esp\v5.5.1\esp-idf",
    "$env:USERPROFILE\esp\esp-idf",
    "C:\esp\esp-idf"
)

$idf_path = $null
foreach ($path in $possible_paths) {
    if (Test-Path "$path\export.ps1") {
        $idf_path = $path
        break
    }
}

if ($null -eq $idf_path) {
    Write-Host "ERROR: ESP-IDF not found in common locations!" -ForegroundColor Red
    Write-Host "Please edit this script and set the correct path." -ForegroundColor Yellow
    exit 1
}

Write-Host "Found ESP-IDF at: $idf_path" -ForegroundColor Green

# Run the export script
& "$idf_path\export.ps1"

Write-Host "`nESP-IDF environment ready!" -ForegroundColor Green
Write-Host "You can now use: idf.py build, idf.py flash, etc." -ForegroundColor Cyan

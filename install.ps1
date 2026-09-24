$ErrorActionPreference = 'Stop'
$installPath = "$env:LOCALAPPDATA\RawDrive"
$exePath = "$installPath\RawDrive.exe"

Write-Host "Instalando RawDrive Engine..."
if (!(Test-Path $installPath)) {
    New-Item -ItemType Directory -Force -Path $installPath | Out-Null
}

# Detener si esta corriendo
Stop-Process -Name RawDrive -Force -ErrorAction SilentlyContinue

# Copiar archivos
Copy-Item ".\bin\RawDrive.exe" -Destination $installPath -Force
if (!(Test-Path "$installPath\config.ini")) {
    Copy-Item ".\config.ini" -Destination $installPath -Force
}
if (!(Test-Path "$installPath\videos")) {
    New-Item -ItemType Directory -Force -Path "$installPath\videos" | Out-Null
}
Copy-Item ".\videos\*" -Destination "$installPath\videos" -Force -Recurse

if (!(Test-Path "$installPath\tools")) {
    New-Item -ItemType Directory -Force -Path "$installPath\tools" | Out-Null
}
Copy-Item ".\tools\*" -Destination "$installPath\tools" -Force -Recurse

# Registro de inicio automatico
$registryPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run"
$name = "RawDriveEngine"
$value = "`"$exePath`""
Set-ItemProperty -Path $registryPath -Name $name -Value $value

# Crear acceso directo en el escritorio
$WshShell = New-Object -comObject WScript.Shell
$Shortcut = $WshShell.CreateShortcut("$env:USERPROFILE\Desktop\RawDrive Engine.lnk")
$Shortcut.TargetPath = $exePath
$Shortcut.WorkingDirectory = $installPath
$Shortcut.IconLocation = "$exePath,0"
$Shortcut.Save()

Write-Host "Instalando dependencias de Inteligencia Artificial (Python)..."
try {
    # Check if python is in PATH
    $pythonPath = (Get-Command python -ErrorAction Stop).Source
    Start-Process -FilePath "python" -ArgumentList "-m pip install imageio[ffmpeg] numpy requests" -Wait -NoNewWindow
    Write-Host "[OK] Dependencias de IA instaladas."
} catch {
    Write-Host "[WARNING] No se detectó Python. La generación de IA requerirá instalación manual."
}

Write-Host "Instalacion completada con exito."
Write-Host "Iniciando motor..."
Start-Process $exePath -WorkingDirectory $installPath

Read-Host "Presiona Enter para salir"

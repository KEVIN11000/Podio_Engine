@echo off
echo --- RawDrive Updater ---
echo [*] Descargando ultimas actualizaciones del repositorio...
git pull

echo [*] Recompilando motor (si hay cambios en C)...
call build.bat

echo [*] Aplicando actualizacion al sistema...
call install.bat

echo [OK] RawDrive Engine actualizado correctamente!
pause

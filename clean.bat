@echo off
for /r %%f in (*.o64) do del "%%f"
for /r %%f in (*.pp.asm) do del "%%f"
for /r %%f in (*.sym) do del "%%f"
for /r %%f in (*.sym1) do del "%%f"
for /r %%f in (*.sym2) do del "%%f"
for /r %%f in (*.srcmap) do del "%%f"
for /r %%f in (*~.*) do del "%%f"
for /r %%f in (*~ *) do del "%%f"
:: Remove obj directories
for /d /r %%d in (obj) do if exist "%%d" rd /s /q "%%d"

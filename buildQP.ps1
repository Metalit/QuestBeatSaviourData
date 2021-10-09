# Builds a .qmod file for loading with QP
& $PSScriptRoot/build.ps1

$ArchiveName = "BeatSaviorJustData.qmod"
$TempArchiveName = "BeatSaviorData.qmod.zip"

Compress-Archive -Path "./libs/arm64-v8a/libbeatsaviordata.so", ".\extern\libbeatsaber-hook_2_3_2.so", ".\mod.json" -DestinationPath $TempArchiveName -Force
Move-Item $TempArchiveName $ArchiveName -Force
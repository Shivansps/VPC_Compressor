reg add "HKCR\*\shell\VPCC" /ve /d "VPC Compressor -> Compress"
reg add "HKCR\*\shell\VPCC" /v Icon /t REG_EXPAND_SZ /d "%~dp0\VPCompressor.exe"
reg add "HKCR\*\shell\VPCC" /v MultiSelectModel /t REG_SZ /d "Player"
reg add "HKCR\*\shell\VPCC\command" /ve /d "%~dp0\VPCompressor.exe /wincomp %%1"

reg add "HKCR\*\shell\VPCD" /ve /d "VPC Compressor -> Decompress"
reg add "HKCR\*\shell\VPCD" /v Icon /t REG_EXPAND_SZ /d "%~dp0\VPCompressor.exe"
reg add "HKCR\*\shell\VPCD" /v MultiSelectModel /t REG_SZ /d "Player"
reg add "HKCR\*\shell\VPCD\command" /ve /d "%~dp0\VPCompressor.exe /windecomp %%1"

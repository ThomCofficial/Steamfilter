# SteamFilter Windows VST3 — unsigned engineering preview

This prerelease contains a compiled Windows x64 VST3 bundle from the exact GitHub commit shown in the release. It passed the automated Windows/Linux DSP tests and the Windows VST3 compilation check.

**This is NOT a signed installer or a finished, externally validated restoration plugin.** It has not undergone comprehensive Cubase host stress tests, pluginval validation, or professional vocal listening trials. Some functionality may still need improvement.

Download `SteamFilter-Win64-PREVIEW.zip` from this repository's official Releases page. `SHA256SUMS.txt` contains the SHA-256 digest of that exact ZIP; match it to the downloaded file before attempting to install or test. On Windows, PowerShell: `Get-FileHash .\SteamFilter-Win64-PREVIEW.zip -Algorithm SHA256`.

**Browser security:** Chrome may warn that a brand-new ZIP containing an unsigned binary is suspicious or uncommon. A checksum or successful build is not proof that the binary is benign, and GitHub hosting does not override Safe Browsing. Do not disable browser protection just to obtain it. If Chrome still blocks the ZIP, the proper next step is signing with a trusted certificate and reviewing any specific security detection rather than hiding or renaming the payload to evade scanning.

If validated and obtained safely: extract the directory `SteamFilter.vst3` as a whole to `C:\Program Files\Common Files\VST3\`, rescan in Cubase, and test on a copy of a session. Report crashes, unusual CPU spikes or artifacts to Issues.

# gmcl_tts

A module for Garry's Mod that provides text-to-speech interfaces to developers.

## Compiling

This is Windows only (uses Microsoft's TTS capabilities).
The only supported compilation platform for this project on Windows are the ones provided by **Visual Studio**.  
If stuff starts erroring or fails to work, be sure to check the correct line endings (\n and such) are present in the files.  

## Requirements

This project requires [garrysmod\_common][1], a framework to facilitate the creation of compilations files (Visual Studio, make, XCode, etc). Simply set the environment variable '**GARRYSMOD\_COMMON**' or the premake option '**gmcommon**' to the path of your local copy of [garrysmod\_common][1].  

  [1]: https://github.com/danielga/garrysmod_common

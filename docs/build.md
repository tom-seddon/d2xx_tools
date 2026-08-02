**This repo has submodules**. Clone it with `--recursive`:

    git clone --recursive https://github.com/tom-seddon/fdload_adfsl
	
If you already cloned it before reading that: don't worry. You can fix
it. Change to the working copy and do this:

	git submodule update --init --recursive

# Building

You can build this on Windows.

## Windows

Prerequisites:

- Visual Studio 2022 (any edition should be good enough)
- Visual Studio 2022's cmake (install it via the installer)
- Python 3.x (the one that comes with Visual Studio 2022 is fine)
  
Process:

Change to the working copy folder and run:

    py -3 configure.py
	
This should complete with no obvious errors.

Run Visual Studio 2022 and load in the appropriate solution:

- `build\vs2022.x64\d2xx_tools.sln` (64-bit)
- `build\vs2022.Win32\d2xx_tools.sln` (32-bit)

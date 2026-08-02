NAME=$(error must specify NAME)
VSVER=$(error must specify VSVER)
VSYEAR=$(error must specify VSYEAR)

VSPATH:=$(shell "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version $(VSVER) -property installationPath)

PATH32:=build/release/vs$(VSYEAR).Win32
PATH64:=build/release/vs$(VSYEAR).x64

.PHONY:release
release:
	py -3 "configure.py" --clean -o "build/release" --version-string "$(NAME)"
	$(MAKE) hello
	"$(VSPATH)\Common7\IDE\devenv.com" /build RelWithDebInfo "$(PATH32)/d2xx_tools.sln"
	"$(VSPATH)\Common7\IDE\devenv.com" /build RelWithDebInfo "$(PATH64)/d2xx_tools.sln"
	zip -9 -j "build/d2xx_tools.Win32.$(NAME).zip" "$(PATH32)/src/d2xx_list/RelWithDebInfo/d2xx_list.exe" "$(PATH32)/src/d2xx_send/RelWithDebInfo/d2xx_send.exe" "$(PATH32)/src/d2xx_recv/RelWithDebInfo/d2xx_recv.exe"
	zip -9 -j "build/d2xx_tools.x64.$(NAME).zip" "$(PATH64)/src/d2xx_list/RelWithDebInfo/d2xx_list.exe" "$(PATH64)/src/d2xx_send/RelWithDebInfo/d2xx_send.exe" "$(PATH64)/src/d2xx_recv/RelWithDebInfo/d2xx_recv.exe"

.PHONY:hello
hello:
	echo hello

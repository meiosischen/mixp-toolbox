!if !$d(BCB)
BCB = $(MAKEDIR)\..
!endif
!if !$d(CFG)
CFG = Release
!endif
!if "$(CFG)" == "Release"
OPT = -O2
LDBG = -v-
!elif "$(CFG)" == "Debug"
OPT = -Od -v -y
LDBG = -v
!else
!error CFG must be Release or Debug
!endif

OUT = build\$(CFG)
CFLAGS = -c -tW -tWM -a8 -H- -Vx -Ve $(OPT) -w-par -D_NO_VCL;WIN32;UNICODE;_UNICODE;WINVER=0x0501;_WIN32_WINNT=0x0501;_WIN32_IE=0x0501
INCLUDES = -I"$(BCB)\Include" -I"src"
LIBPATH = -L"$(BCB)\Lib" -L"$(BCB)\Lib\Psdk" -L"$(BCB)\Lib\Obj"
OBJECTS = $(OUT)\MixpToolbox.obj $(OUT)\MainWindow.obj $(OUT)\MenuRegistry.obj

all: $(OUT)\mixp-toolbox.exe

$(OUT)\MixpToolbox.obj: MixpToolbox.cpp src\MainWindow.h src\MenuRegistry.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) $(INCLUDES) -o$@ MixpToolbox.cpp

$(OUT)\MainWindow.obj: src\MainWindow.cpp src\MainWindow.h src\MenuRegistry.h src\resource.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) $(INCLUDES) -o$@ src\MainWindow.cpp

$(OUT)\MenuRegistry.obj: src\MenuRegistry.cpp src\MenuRegistry.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) $(INCLUDES) -o$@ src\MenuRegistry.cpp

$(OUT)\MixpToolbox.res: MixpToolbox.rc src\resource.h resources\MixpToolbox.manifest
	"$(BCB)\Bin\brcc32.exe" -i"$(BCB)\Include" -fo$@ MixpToolbox.rc

$(OUT)\mixp-toolbox.exe: $(OBJECTS) $(OUT)\MixpToolbox.res
	"$(BCB)\Bin\ilink32.exe" -Tpe -aa -x -Gn $(LDBG) $(LIBPATH) c0w32.obj $(OBJECTS),$@,,import32.lib cw32mt.lib,,$(OUT)\MixpToolbox.res

$(OUT)\RegistryTests.obj: tests\RegistryTests.cpp src\MenuRegistry.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) $(INCLUDES) -o$@ tests\RegistryTests.cpp

$(OUT)\registry-tests.exe: $(OUT)\RegistryTests.obj $(OUT)\MenuRegistry.obj
	"$(BCB)\Bin\ilink32.exe" -Tpe -ap -x -Gn $(LDBG) $(LIBPATH) c0x32.obj $(OUT)\RegistryTests.obj $(OUT)\MenuRegistry.obj,$@,,import32.lib cw32mt.lib,,

$(OUT)\TestMain.obj: MixpToolbox.cpp src\MainWindow.h src\MenuRegistry.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) -DMIXP_TEST_BUILD $(INCLUDES) -o$@ MixpToolbox.cpp

$(OUT)\TestWindow.obj: src\MainWindow.cpp tests\WindowCapture.h src\MainWindow.h src\MenuRegistry.h src\resource.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) -DMIXP_TEST_BUILD $(INCLUDES) -o$@ src\MainWindow.cpp

$(OUT)\mixp-toolbox-test.exe: $(OUT)\TestMain.obj $(OUT)\TestWindow.obj $(OUT)\MenuRegistry.obj $(OUT)\MixpToolbox.res
	"$(BCB)\Bin\ilink32.exe" -Tpe -aa -x -Gn $(LDBG) $(LIBPATH) c0w32.obj $(OUT)\TestMain.obj $(OUT)\TestWindow.obj $(OUT)\MenuRegistry.obj,$@,,import32.lib cw32mt.lib,,$(OUT)\MixpToolbox.res

$(OUT)\GuiTests.obj: tests\GuiTests.cpp src\MenuRegistry.h src\resource.h
	"$(BCB)\Bin\bcc32.exe" $(CFLAGS) $(INCLUDES) -o$@ tests\GuiTests.cpp

$(OUT)\gui-tests.exe: $(OUT)\GuiTests.obj $(OUT)\MenuRegistry.obj
	"$(BCB)\Bin\ilink32.exe" -Tpe -ap -x -Gn $(LDBG) $(LIBPATH) c0x32.obj $(OUT)\GuiTests.obj $(OUT)\MenuRegistry.obj,$@,,import32.lib cw32mt.lib,,

tests: $(OUT)\registry-tests.exe $(OUT)\mixp-toolbox-test.exe $(OUT)\gui-tests.exe

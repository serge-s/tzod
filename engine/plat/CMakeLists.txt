add_library(plat
	inc/plat/AppWindow.h
	inc/plat/Clipboard.h
	inc/plat/ConsoleBuffer.h
	inc/plat/Folders.h
	inc/plat/Input.h
	inc/plat/Keys.h

	ConsoleBuffer.cpp
)

target_link_libraries(plat
	INTERFACE math
)

target_include_directories(plat INTERFACE inc)
set_target_properties(plat PROPERTIES FOLDER engine)
set(video_SOURCES
	inc/video/EditableImage.h
	inc/video/ImageView.h
	inc/video/RenderBase.h
	inc/video/RenderBinding.h
	inc/video/RenderContext.h
	inc/video/RingAllocator.h
	inc/video/TextureManager.h
	inc/video/TexturePackage.h
	inc/video/TgaImage.h

	AtlasPacker.cpp
	AtlasPacker.h
	EditableImage.cpp
	RenderBinding.cpp
	RenderContext.cpp
	TextureManager.cpp
	TexturePackage.cpp
	TgaImage.cpp
)

if(IOS OR ANDROID)
	list(APPEND video_SOURCES
		inc/video/RenderGLES2.h
		inc/video/detail/GLESProgram.h
		GLESProgram.cpp
		RenderGLES2.cpp
	)
elseif(WINRT)
	list(APPEND video_SOURCES
		inc/video/RenderD3D11.h
		inc/video/RenderD3D12.h
		RenderD3D11.cpp
		RenderD3D12.cpp
	)
else()
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		RenderOpenGL.cpp
	)
endif()

add_library(video ${video_SOURCES})

target_link_libraries(video PRIVATE
	fs
	lua
	luaetc
	math
)

target_include_directories(video PRIVATE
	# 3rd party
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(video INTERFACE inc)
set_target_properties(video PROPERTIES FOLDER engine)
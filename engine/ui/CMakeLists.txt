add_library(ui
	Button.cpp
	Combo.cpp
	Console.cpp
	DataSource.cpp
	Dialog.cpp
	Edit.cpp
	EditableText.cpp
	GuiManager.cpp
	InputContext.cpp
	LayoutContext.cpp
	List.cpp
	ListBase.cpp
	ListBox.cpp
	ListSelectionBinding.cpp
	MultiColumnListItem.cpp
	Navigation.cpp
	Rating.cpp
	Rectangle.cpp
	ScanlineLayout.cpp
	ScrollBar.cpp
	ScrollView.cpp
	StackLayout.cpp
	Text.cpp
	Texture.cpp
	Window.cpp

	inc/ui/Button.h
	inc/ui/Combo.h
	inc/ui/Console.h
	inc/ui/DataContext.h
	inc/ui/DataSource.h
	inc/ui/DataSourceAdapters.h
	inc/ui/Dialog.h
	inc/ui/Edit.h
	inc/ui/EditableText.h
	inc/ui/GuiManager.h
	inc/ui/InputContext.h
	inc/ui/LayoutContext.h
	inc/ui/List.h
	inc/ui/ListBase.h
	inc/ui/ListBox.h
	inc/ui/ListSelectionBinding.h
	inc/ui/MultiColumnListItem.h
	inc/ui/Navigation.h
	inc/ui/PointerInput.h
	inc/ui/Rating.h
	inc/ui/Rectangle.h
	inc/ui/ScanlineLayout.h
	inc/ui/ScrollBar.h
	inc/ui/ScrollView.h
	inc/ui/StackLayout.h
	inc/ui/StateContext.h
	inc/ui/Text.h
	inc/ui/Texture.h
	inc/ui/Window.h
	inc/ui/WindowIterator.h
)

target_link_libraries(ui PUBLIC
	math
	video

	PRIVATE plat
)

target_include_directories(ui INTERFACE inc)
set_target_properties(ui PROPERTIES FOLDER engine)
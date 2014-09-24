#pragma once

#include <ui/UIInput.h>
#include <ui/Clipboard.h>
#include <memory>

struct GLFWwindow;

class GlfwInput : public UI::IInput
{
public:
	GlfwInput(GLFWwindow &window);
	
	// UI::IInput
	virtual bool IsKeyPressed(int key) const override;
	virtual bool IsMousePressed(int button) const override;
	virtual vec2d GetMousePos() const override;
	
private:
	GLFWwindow &_window;
};

class GlfwClipboard : public UI::IClipboard
{
public:
	GlfwClipboard(GLFWwindow &window);
	
	// UI::IClipboard
	virtual const char* GetClipboardText() const override;
	virtual void SetClipboardText(std::string text) override;
	
private:
	GLFWwindow &_window;
};

struct GlfwInitHelper
{
	GlfwInitHelper();
	~GlfwInitHelper();
};

struct GlfwWindowDeleter
{
	void operator()(GLFWwindow *window);
};

class GlfwAppWindow
{
public:
	GlfwAppWindow(const char *title, bool fullscreen, int width, int height);
	~GlfwAppWindow();
	GLFWwindow& GetGlfwWindow() const { return *_window; }
	
private:
	GlfwInitHelper _initHelper;
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> _window;
};
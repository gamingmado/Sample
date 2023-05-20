#pragma once

class Window
{
public:
	Window(HINSTANCE hInstance, int nCmdShow);
	virtual ~Window();

	int Run();

private:
	void Initialize(HINSTANCE hInstance, int nCmdShow);
};

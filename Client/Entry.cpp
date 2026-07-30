#include <App/PDMainApplication.h>

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	PDMainApplication app;

	return app.run(instance);
}

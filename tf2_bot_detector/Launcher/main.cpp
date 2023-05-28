#include "../DLLMain.h"

#ifdef WIN32
#include <Windows.h>
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nShowCmd)
{
	return tf2_bot_detector::RunProgram(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
}
#else
int main(int argc, const char** argv)
{
	return tf2_bot_detector::RunProgram(argc, argv);
}
#endif



#include "UI.h"
#include "ServerAPI.h"

demonorium::Server demonorium::ServerAPI::server("valid cd",3333);

int main() {
	demonorium::ServerAPI::init();
	
	std::setlocale(LC_ALL, "RU");
	demonorium::Window window(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse,
		3333,true);
	demonorium::ServerAPI::terminate();
}
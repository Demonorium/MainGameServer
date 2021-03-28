#include "UI.h"
#include "ServerAPI.h"

demonorium::Server demonorium::ServerAPI::server("valid cd",3333);

int main() {
	std::setlocale(LC_ALL, "RU");
	
	demonorium::ServerAPI::init();
	while (!demonorium::ServerAPI::is_launched());
	
	
	demonorium::Window window(ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse,
		3333,true);
	demonorium::ServerAPI::terminate();
}
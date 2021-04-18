#pragma once


#include "imgui.h"
#include "imgui-SFML.h"
#include <array>
#include <SFML\Graphics.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include "ServerAPI.h"
#include "Player.h"

namespace demonorium
{
	
	class Window {
		sf::RenderWindow m_window;
		ImGuiWindowFlags m_flags;
		sf::Clock m_delta_clock;

		std::array<char, 32> m_port_input_buffer;
		std::array<char, 8> m_ip0, m_ip1, m_ip2, m_ip3;
		
		
		
		void processEvents();
		void screen();
		void guiRender();
	public:
		Window(ImGuiWindowFlags flags, unsigned short defaultPort, bool start = false);
		~Window();
		
		void loop();
		sf::RenderWindow& getSfWindow();
	};


	inline void Window::processEvents() {
		sf::Event event;
		while (m_window.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(event);

			if (event.type == sf::Event::Closed) {
				m_window.close();
			}
		}
	}
	namespace
	{
		static constexpr char C_YES[] = "yes";
		static constexpr char C_NO[] = "no";
	}

	
	inline void Window::screen() {
		ImGui::SetNextWindowSize(m_window.getSize());
		ImGui::SetNextWindowPos({ 0,0 });

		if (ImGui::Begin("Screen", nullptr, m_flags))
		{
			if (ImGui::BeginTable("Base table", 2)) {
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				
				
				if (ImGui::BeginListBox("Player List", { static_cast<float>(m_window.getSize().x / 2), static_cast<float>(m_window.getSize().y * 0.9)  })) {
					if (ImGui::BeginTable("Players", 6, ImGuiTableFlags_Borders)) {
						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						ImGui::Text("IP");
						
						ImGui::TableNextColumn();
						ImGui::Text("NAME");

						ImGui::TableNextColumn();
						ImGui::Text("READY");
						

						ImGui::TableNextColumn();
						ImGui::Text("KILLS");

						ImGui::TableNextColumn();
						ImGui::Text("D-TIME");
						
						ImGui::TableNextColumn();
						ImGui::Text("KILLED BY");

						
						ImGui::TableNextRow();
						std::string UI_NAME = "Port control";
						int i = 0;
						
						for (const auto& bundle : ServerAPI::getPlayerList()) {
							ImGui::TableNextColumn();
							ImGui::Text( bundle.first.toString().c_str());

							ImGui::TableNextColumn();
							ImGui::Text(bundle.second.getName().c_str());

							ImGui::TableNextColumn();
							ImGui::Text(bundle.second.isReady() ? C_YES : C_NO);

							ImGui::TableNextColumn();
							ImGui::Text(std::to_string(bundle.second.getKillCounter()).c_str());

							if (!bundle.second.alive()) {
								ImGui::TableNextColumn();
								ImGui::Text(std::to_string(std::chrono::duration_cast<chrono::seconds>(bundle.second.getDieTime() - ServerAPI::get_game_start_time()).count()).c_str());
								
								ImGui::TableNextColumn();
								ImGui::Text(bundle.second.getKillerIP().toString().c_str());
							}
							
							
							
							ImGui::TableNextRow();
							++i;
						}
						ImGui::EndTable();
					}
					
					ImGui::EndListBox();
				}


				ImGui::TableNextColumn();
				if (ImGui::BeginTable("Port control", 2, ImGuiTableFlags_BordersOuter)) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::InputText("Port", m_port_input_buffer.data(), 8)) {
						for (char& c : m_port_input_buffer)
							if (!std::isdigit(c) && (c != '\0'))
								c = '0';
								
					}
					ImGui::TableNextColumn();
					ImGui::Text("Current: %i", ServerAPI::current_port());
					
					ImGui::TableNextRow();
					ImGui::TableNextColumn();

					
					if (ImGui::Button("Update")) {
						ServerAPI::update_port(std::atoi(m_port_input_buffer.data()));
					}
					
					ImGui::EndTable();
				}
				if (ImGui::BeginTable("IP control", 7, ImGuiTableFlags_BordersOuter)) {
					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					if (ImGui::InputText("##1", m_ip0.data(), 4)) {
						for (char& c : m_ip0)
							if (!std::isdigit(c) && (c != '\0'))
								c = '0';
					}
					ImGui::TableNextColumn();
					ImGui::Text(".");
					ImGui::TableNextColumn();
					if (ImGui::InputText("##2", m_ip1.data(), 4)) {
						for (char& c : m_ip1)
							if (!std::isdigit(c) && (c != '\0'))
								c = '0';
					}
					ImGui::TableNextColumn();
					ImGui::Text(".");
					ImGui::TableNextColumn();
					if (ImGui::InputText("##3", m_ip2.data(), 4)) {
						for (char& c : m_ip2)
							if (!std::isdigit(c) && (c != '\0'))
								c = '0';
					}
					ImGui::TableNextColumn();
					ImGui::Text(".");
					ImGui::TableNextColumn();
					if (ImGui::InputText("##4", m_ip3.data(), 4)) {
						for (char& c : m_ip3)
							if (!std::isdigit(c) && (c != '\0'))
								c = '0';
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();


					if (ImGui::Button("Update")) {
						ServerAPI::set_ip_alias(
							std::atoi(m_ip0.data()), std::atoi(m_ip1.data()), 
							std::atoi(m_ip2.data()), std::atoi(m_ip3.data()));
					}

					ImGui::EndTable();
				}
				ImGui::Text((std::string("password: '") + ServerAPI::get_password() + "'").c_str());
				
				if (ImGui::Button("START(RESTART) GAME")) {
					ServerAPI::restart_game();
				}
				if (ImGui::Button("FORCE START(RESTART)")) {
					ServerAPI::force_restart_game();
				}
				
				if (ImGui::Button("CLEAR PLAYER LIST")) {
					ServerAPI::reset();
				}
				
				
				ImGui::TableNextRow();
				ImGui::EndTable();

			}
			ImGui::End();
		}
	}

	inline void Window::guiRender() {
		m_window.clear();
		ImGui::SFML::Render(m_window);
		m_window.display();
	}

	inline Window::Window(ImGuiWindowFlags flags, unsigned short defaultPort, bool start):
		m_window(sf::VideoMode(1024, 640), "Game server UI"),
		m_flags(flags) {

		for (auto& ch : m_port_input_buffer)
			ch = '\0';

		for (auto& ch : m_ip0)
			ch = '\0';
		for (auto& ch : m_ip1)
			ch = '\0';
		for (auto& ch : m_ip2)
			ch = '\0';
		for (auto& ch : m_ip3)
			ch = '\0';
		
		auto temp_string = std::to_string(defaultPort);
		std::string temp_ip_0 = "127";
		std::string temp_ip_12 = "0";
		std::string temp_ip_3 = "1";

		std::memcpy(m_port_input_buffer.data(), temp_string.c_str(), temp_string.size());
		std::memcpy(m_ip0.data(), temp_ip_0.data(),		temp_ip_0.size());
		std::memcpy(m_ip1.data(), temp_ip_12.data(),	temp_ip_12.size());
		std::memcpy(m_ip2.data(), temp_ip_12.data(),	temp_ip_12.size());
		std::memcpy(m_ip3.data(), temp_ip_3.data(),		temp_ip_3.size());
		
		ImGui::SFML::Init(m_window);
		if (start)
			loop();
	}

	inline Window::~Window() {
		ImGui::SFML::Shutdown();
	}

	inline void Window::loop() {
		sf::Clock deltaClock;
		while (m_window.isOpen()) {
			processEvents();
			ImGui::SFML::Update(m_window, deltaClock.restart());
			screen();
			guiRender();
		}
	}

	inline sf::RenderWindow& Window::getSfWindow() {
		return m_window;
	}
}

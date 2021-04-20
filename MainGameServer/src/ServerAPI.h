#pragma once

#include <set>


#include "Player.h"
#include "Server.h"


namespace demonorium
{
	class ServerAPI {
		friend class Server;
		static Server server;
	public:
		//��������� �������������, ������ ������ �������
		static void init();
		//��������� ����������
		static void terminate();
		//���������� true ����� ������ ������������ �������
		static bool is_launched();
		
		//����� ����� ����� ��������: �������� ����� ������
		static void update_port(sf::Uint16 port);
		//���������� ������� ����
		static sf::Uint16 get_port();
		
		//����� IP ��� 127.0.0.1
		static void set_ip_alias(byte ip0, byte ip1, byte ip2, byte ip3);
		
		//���������� ������� ������
		static const char* get_password();

		//���������� ����� ������ ��������� ����
		static auto get_game_start_time();
		
		//���������� std::map ���������� ���� ������� �� �� ip
		static const auto& get_player_list();
		
		//����������� �������� �������� � �������
		static void request(UserRequest request);
	};


	inline void ServerAPI::init() {
		server.m_launched.store(false);
		server.start();
	}

	inline void ServerAPI::update_port(sf::Uint16 port) {
		server.m_input_thread.setPort(port);
	}

	inline void ServerAPI::set_ip_alias(byte ip0, byte ip1, byte ip2, byte ip3) {
		server.m_host.alias.store(sf::IpAddress(ip0, ip1, ip2, ip3));
	}

	inline const char* ServerAPI::get_password() {
		return server.m_password.password;
	}

	inline sf::Uint16 ServerAPI::get_port() {
		return server.m_input_thread.getPort();
	}

	inline auto ServerAPI::get_game_start_time() {
		return server.m_chrono.game_start;
	}

	inline void ServerAPI::terminate() {
		server.destroyThread();
	}

	inline const auto& ServerAPI::get_player_list() {
		return server.m_players;
	}

	inline bool ServerAPI::is_launched() {
		return server.m_launched.load();
	}

	inline void ServerAPI::request(UserRequest request) {
		server.request(request);
	}
}

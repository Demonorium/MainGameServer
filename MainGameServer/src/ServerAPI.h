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

		//����� ����� ����� ��������: �������� ����� ������
		static void update_port(unsigned port);

		//����� IP ��� 127.0.0.1
		static void set_ip_alias(byte ip0, byte ip1, byte ip2, byte ip3);
		
		//���������� ������� ������
		static const char* get_password();
		//���������� ������� ����
		static unsigned current_port();
		//������������� ����
		static void restart_game();
		//������������� �������� ����
		static void force_restart_game();
		//������ �����
		static void reset();
		static auto get_game_start_time();
		
		static void terminate();
		static const auto& getPlayerList();
		static bool is_launched();
	};


	inline void ServerAPI::init() {
		server.m_launched.store(false);
		server.start();
	}

	inline void ServerAPI::update_port(unsigned port) {
		server.m_input_thread.setPort(port);
	}

	inline void ServerAPI::set_ip_alias(byte ip0, byte ip1, byte ip2, byte ip3) {
		server.m_host.alias.store(sf::IpAddress(ip0, ip1, ip2, ip3));
	}

	inline const char* ServerAPI::get_password() {
		return server.m_password.password;
	}

	inline unsigned ServerAPI::current_port() {
		return server.m_input_thread.getPort();
	}

	inline void ServerAPI::restart_game() {
		server.restartGame();
	}

	inline void ServerAPI::force_restart_game() {
		server.forceRestart();
	}

	inline void ServerAPI::reset() {
		server.destroyThread();
		server.m_players.clear();
		server.start();
	}

	inline auto ServerAPI::get_game_start_time() {
		return server.m_chrono.game_start;
	}

	inline void ServerAPI::terminate() {
		server.destroyThread();
	}

	inline const auto& ServerAPI::getPlayerList() {
		return server.m_players;
	}

	inline bool ServerAPI::is_launched() {
		return server.m_launched.load();
	}
}

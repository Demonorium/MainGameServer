#pragma once

#include <set>

#include "BaseThread.h"
#include "InputThread.h"
#include "Player.h"

#include "Packet.h"
#include "ServerAPI.h"
namespace demonorium
{
	class Server : public BaseThread {
		friend class ServerAPI;

		std::chrono::system_clock::time_point m_game_start;

		std::atomic<sf::IpAddress> m_ip_alias;
		
		sf::UdpSocket m_output;
		InputThread m_input_thread;
		std::map<sf::IpAddress, Player> m_players;

		bool m_game_started;
		char m_password[9];
		bool m_need_restart;
		bool m_internal_restart;
		std::atomic<bool> m_launched;

		
		bool isValidPassword(const char pas[8]) const;

		//��������� ������ �� ip � port 
		template<class ... Args>
		void send(sf::IpAddress address, sf::Uint16 port, byte code, Args&& ...);
		template<class T, class ... Args>
		void send(Packet& pack, sf::IpAddress address, sf::Uint16 port, const T& a, Args&& ... args);
		void send(Packet& pack, sf::IpAddress address, sf::Uint16 port);
	public:
		explicit Server(const char password[9], unsigned short port = 3333);

		void onInit() override;
		void onPause() override;
		void onFrame() override;
		void onUnPause() override;
		void onDestruction() override;

		void restartGame();
		void forceRestart();
	};

	template <class ... Args>
	void Server::send(sf::IpAddress address, sf::Uint16 port, byte code, Args&&... args) {
		Packet packet(255);
		send(packet, address, port, code, std::forward<Args>(args)...);
	}

	template <class T, class ... Args>
	void Server::send(Packet& pack, sf::IpAddress address, sf::Uint16 port, const T& a, Args&&... args) {
		pack.write(a);
		send(pack, address, port, std::forward<Args>(args)...);
	}
	
	void Server::onInit() {
		m_input_thread.start();
		m_output.bind(sf::Socket::AnyPort);
		std::cout << "�������� � �����: " << m_output.getLocalPort() << std::endl;
		m_launched.store(true);
	}
	
	inline void Server::send(Packet& pack, sf::IpAddress address, sf::Uint16 port) {
		std::cout << "Send: '";
		for (int i = 0; i < pack.size(); ++i)
			std::cout << static_cast<size_t>(static_cast<byte*>(pack.data())[i]) << ' ';
		std::cout << "' to " << address << ":" << port << std::endl;
		
		m_output.send(pack.data(), pack.size(), address, port);
	}


	inline bool Server::isValidPassword(const char pas[8]) const {
		for (int i = 0; i < 8; ++i)
			if (m_password[i] != pas[i])
				return false;
		return true;
	}

	inline Server::Server(const char password[9], unsigned short port):
		m_input_thread(port, 255, 128), m_game_started(false),
		m_need_restart(false), m_internal_restart(false), m_ip_alias(sf::IpAddress(127, 0, 0, 1)) {
		std::memcpy(m_password, password, 9);
	}

	inline void Server::onPause() {
		m_input_thread.pause();
		m_game_started = false;
	}

	inline void Server::onFrame() {
		void* received_memory = m_input_thread.get();

		bool nopack = false;
		if (received_memory != nullptr) {
			//C��������� ������ �� ������� � �������� ���������������� �������
			PacketPrefix prefix = *static_cast<PacketPrefix*>(received_memory);
			if (prefix.ip == sf::IpAddress::LocalHost)
				prefix.ip = m_ip_alias.load();
			
			received_memory = memory_shift(received_memory, sizeof(PacketPrefix));
			Packet pack(received_memory, prefix.size);

			
			
			//���� ���� ����� ������������ ����� �������, ����� ������������
			if (pack.enoughMemory<byte>()) {
				const byte code = *pack.read<byte>();

				
				//BODY
				auto iterator = m_players.find(prefix.ip);
				if (iterator == m_players.end()) {
					std::cout << "������� ������ �� ������������ ������������: " << code << '\n';
					std::cout << '\t' << prefix.ip << std::endl;

					
					if (!m_game_started && (code == 0) && (pack.enoughMemoryMany<sf::Uint16>(8u))) {
						char pas[8];
						const char* pas_raw = pack.read<char>(8);
						std::memcpy(pas, pas_raw, 8);

						if (isValidPassword(pas)) {
							const unsigned short send_port = *pack.read<sf::Uint16>();
							const size_t size              = pack.availableSpace();
							std::string name(pack.read<char>(size), size);
							m_players.insert(iterator, std::make_pair(prefix.ip, Player(send_port, std::move(name))));
							send(prefix.ip, send_port, 4, prefix.ip);
						}
					} else {
						std::cout << "������. ������������� ������ ������ �������\n" << std::endl;
					}
				}
				else {
					switch (code) {
					case 0: //���������������
						if (!m_game_started && (pack.enoughMemoryMany<sf::Uint16>(8u))) {
							char pas[8];
							const char* pas_raw = pack.read<char>(8);
							std::memcpy(pas, pas_raw, 8);

							if (isValidPassword(pas)) {
								const unsigned short send_port = *pack.read<sf::Uint16>();
								const size_t size = pack.availableSpace();
								std::string name(pack.read<char>(size), size);
								std::cout << "���������������: " << iterator->second.getName() << " -> " << name << '\n';
								iterator->second.setPort(send_port);
								iterator->second.setName(std::move(name));
								send(iterator->first, iterator->second.getPort(), 4, iterator->first);
							}
						}
						break;
					case 1: //�������� ������.
						if (!m_game_started)
							m_players.erase(iterator);
						break;
					case 2: //READY
						if (!m_game_started && m_internal_restart) {
							iterator->second.ready();

							m_game_started = true;
							for (const auto& bundle : m_players)
								if (bundle.second.alive() && !bundle.second.isReady()) {
									m_game_started = false;
									break;
								}

							if (m_game_started) {
								m_internal_restart = false;
								m_game_start = chrono::system_clock::now();
								for (const auto& bundle : m_players) {
									if (bundle.second.alive())
										send(bundle.first, bundle.second.getPort(), 1);
								}
							}
						}
						break;
					case 3: //������
						if (m_game_started && iterator->second.alive()) {
							if (pack.enoughMemory<byte>(4)) {
								Packet packet(5);
								packet.write(static_cast<byte>(3));
								packet.write(iterator->first);
								
								int alive_counter = 0;
								for (const auto& bundle : m_players) {
									if (bundle.second.alive()) {
										send(packet, bundle.first, bundle.second.getPort());
										++alive_counter;
									}
								}
								if (alive_counter <= 2) {
									m_game_started = false;
								}

								byte arr[4];
								
								memcpy(arr, pack.read(4), 4);

								sf::IpAddress killer(arr[0], arr[1], arr[2], arr[3]);
								auto killer_player = m_players.find(killer);
								if (killer_player != m_players.end()) {
									killer_player->second.incKillCounter();
								}
								else {
									killer = iterator->first;
								}
								iterator->second.kill(killer);
							}
						}
						break;
					case 4: //������ �������
					{
						size_t count = 252 / 4;
						byte fcount = 0;

						Packet packet(255);

						packet.write(byte(2));
						packet.write(byte(0));
						 
						for (const auto& bundle : m_players) {
							if (bundle.second.alive() && bundle.second.isReady()) {
								packet.write(bundle.first);
								++fcount;
							}
						}
						static_cast<byte*>(packet.data())[1] = fcount;
						send(packet, iterator->first, iterator->second.getPort());
					}
					break;

					default:
						std::cerr << "Unknown code from " << prefix.ip.toString() << "\n";
						nopack = true;
					}
				}	
			}else
				nopack = true;
		} else
			nopack = true;
		
		if (m_need_restart) {
			m_internal_restart = true;
			for (auto& player : m_players) {
				player.second.gameReset();
				send(player.first, player.second.getPort(), 0);
			}
			m_game_started = false;
			m_need_restart = false;
		} else if (nopack)
			std::this_thread::yield();
	}

	inline void Server::onUnPause() {
		m_input_thread.run();
	}

	inline void Server::onDestruction() {
		for (auto& player : m_players) {
			player.second.gameReset();
		}
		m_game_started = false;
		m_internal_restart = false;
		m_need_restart = false;
	}

	inline void Server::restartGame() {
		m_need_restart = true;
	}

	inline void Server::forceRestart() {
		m_internal_restart = false;
		m_game_started = true;
		m_need_restart = false;
		m_game_start = chrono::system_clock::now();
		for (auto& player : m_players) {
			player.second.gameReset();
			player.second.ready();
			send(player.first, player.second.getPort(), 1);
		}
		
	}
}

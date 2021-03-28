#pragma once

#include <set>

#include "BaseThread.h"
#include "InputThread.h"
#include "Player.h"

#include "Packet.h"

namespace demonorium
{
	class Server : public BaseThread {
		friend class ServerAPI;

		std::chrono::system_clock::time_point m_game_start;
		
		sf::UdpSocket m_output;
		InputThread m_input_thread;
		std::map<sf::IpAddress, Player> m_players;

		bool m_game_started;
		char m_password[9];
		bool m_need_restart;
		bool m_internal_restart;
		
		template<class T>
		static void read_from_packet(void*& packet, PacketPrefix& prefix, T& target);
		template<class T>
		static bool enough_memory(const PacketPrefix& prefix);
		bool isValidPassword(const char pas[8]) const;

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


	template <class T>
	void Server::read_from_packet(void*& packet, PacketPrefix& prefix, T& target) {
		target = *static_cast<char*>(packet);
		packet = memory_shift(packet, sizeof(T));
		prefix.size -= sizeof(T);
	}

	template <class T>
	bool Server::enough_memory(const PacketPrefix& prefix) {
		return prefix.size >= sizeof(T);
	}

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

	inline void Server::send(Packet& pack, sf::IpAddress address, sf::Uint16 port) {
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
		m_need_restart(false), m_internal_restart(false) {
		std::memcpy(m_password, password, 9);
	}


	inline void Server::onInit() {
		m_input_thread.start();
		m_output.bind(sf::Socket::AnyPort);
	}

	inline void Server::onPause() {
		m_input_thread.pause();
		m_game_started = false;
	}

	inline void Server::onFrame() {
		void* packet = m_input_thread.get();

		bool nopack = false;
		if (packet != nullptr) {
			//PREFIX
			PacketPrefix prefix = *static_cast<PacketPrefix*>(packet);
			packet = memory_shift(packet, sizeof(PacketPrefix));
			Packet pack(packet, prefix.size);

			
			if (pack.enoughMemory<byte>()) {
				byte code = *pack.read<byte>();

				
				//BODY
				auto iterator = m_players.find(prefix.ip);
				if (iterator == m_players.end()) {
					std::cout << "Получен запрос от неизвестного пользователя: " << code << '\n';
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
						}
					} else {
						std::cout << "Ошибка. Недостаточный размер пакета запроса\n" << std::endl;
					}
				}
				else {
					switch (code) {
					case 0:
						if (!m_game_started && (pack.enoughMemoryMany<sf::Uint16>(8u))) {
							char pas[8];
							const char* pas_raw = pack.read<char>(8);
							std::memcpy(pas, pas_raw, 8);

							if (isValidPassword(pas)) {
								const unsigned short send_port = *pack.read<sf::Uint16>();
								const size_t size = pack.availableSpace();
								std::string name(pack.read<char>(size), size);
								m_players.insert(iterator, std::make_pair(prefix.ip, Player(send_port, std::move(name))));
							}
						}
						break;
					case 1:
						if (!m_game_started)
							m_players.erase(iterator);
						break;
					case 2:
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
					case 3:
						if (m_game_started && iterator->second.alive()) {
							if (4 <= prefix.size) {
								Packet packet(5);
								pack.write(3);
								pack.write(iterator->first.toInteger());
								
								int alive_counter = 0;
								for (const auto& bundle : m_players) {
									if (bundle.second.alive()) {
										
										m_output.send(pack.data(), pack.size(), bundle.first, bundle.second.getPort());
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
					case 4:
					{
						constexpr static byte uint2 = 2;
						size_t count = 252 / 4;
						byte fcount = 0;

						Packet packet(255);

						packet.write(byte(2));
						packet.write(byte(0));
						
						for (const auto& bundle : m_players) {
							if (bundle.second.alive() && bundle.second.isReady()) {
								packet.write(bundle.first.toInteger());
								++fcount;
							}
						}
						static_cast<byte*>(packet.data())[1] = fcount;
						m_output.send(packet.data(), packet.size(), iterator->first, iterator->second.getPort());
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
		}
		
	}
}

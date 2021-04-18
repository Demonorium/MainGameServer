#pragma once

#include <set>

#include "BaseThread.h"
#include "InputThread.h"
#include "Player.h"

#include "Packet.h"

#include <DSFML/Aliases.h>
DEMONORIUM_ALIASES;
DEMONORIUM_LOCAL_USE(demonorium::memory::memory_declarations);

namespace demonorium
{
	struct GameState {
		//Игра в процессе
		std::atomic_bool game_started;
		//Опрос участников на готовность
		std::atomic_bool ready_testing;
		//Игра успешно закончилась
		std::atomic_bool game_ended;
		//Запрос перезапуска
		std::atomic_bool restart_request;

		//Запрос перезапуска
		std::atomic_bool force_request;
		
		void set_default();
		GameState();
	};

	//Хранит ключ сервера и проверяет строки на соответсвие ключу
	struct Password {
		static constexpr const size_t LENGTH = 8;
		static constexpr size_t TERMINATOR_LENGTH = LENGTH + 1;
		char password[TERMINATOR_LENGTH];
		
		bool isValid(const char* string);

		Password(const char* string);
	};

	//Подменяет mask_ip на указанный в alias
	struct IPAlias {
		const sf::IpAddress			mask_ip;
		std::atomic<sf::IpAddress>	alias;

		sf::IpAddress convert(sf::IpAddress ip) const;
		
		IPAlias(const sf::IpAddress& mask);
	};

	struct Chrono {
		using delay		 = std::chrono::milliseconds;
		using clock		 = std::chrono::system_clock;
		using time_point = clock::time_point;
		using crdelay = const delay&;
		
		//Задержка после получения kill перед неспосредственной смертью
		const delay kill_delay;
		//Задержка после которой игрок засчитается замоубившимся
		const delay inactive_delay;
		//Задержка перед предупреждением о бездействии
		const delay warning_delay;
		//Время начала игры
		time_point game_start;

		Chrono(crdelay kill, crdelay inactive, crdelay warning);
	};

	enum class ServerCodes: byte {
		READY_REQ	 = 0,
		GAME_STARTED = 1,
		TABLE		 = 2,
		DEATH		 = 3,
		REGISTER	 = 4,
		RESP_CHECK   = 5,
		GAME_ENDED	 = 6
		
	};

	enum class ClientCodes: byte {
		REGISTER	= 0,
		DELETE		= 1,
		READY		= 2,
		DEATH		= 3,
		TABLE		= 4,
		ACTIVE		= 5,
		NAME		= 6,
		KILL		= 7
	};
	
	class Server : public BaseThread {
		friend class ServerAPI;

		//Проверка, что сервер успешно запущен
		std::atomic<bool> m_launched;
		
		InputThread		m_input_thread;
		GameState		m_state;
		Password		m_password;
		IPAlias			m_host;
		Chrono			m_chrono;
		sf::UdpSocket	m_output;

		std::map<sf::IpAddress, Player> m_players;


		//Пометить начало игры и сообщить игрокам
		void startGame();
		//Закончить игру и сообщить игрокам
		void endGame();
		
		//Отправить данные на ip и port 
		template<class ... Args>
		void send(sf::IpAddress address, sf::Uint16 port, ServerCodes code, Args&& ...);
		template<class T, class ... Args>
		void send(Packet& pack, sf::IpAddress address, sf::Uint16 port, const T& a, Args&& ... args);
		void send(Packet& pack, sf::IpAddress address, sf::Uint16 port);
	public:
		explicit Server(const char password[9], unsigned short port = 3333, 
			Chrono::crdelay kill		= 20s,
			Chrono::crdelay inactive	= 35s,
			Chrono::crdelay warning		= 1s);

		void onInit() override;
		void onPause() override;
		void onFrame() override;
		void onUnPause() override;
		void onDestruction() override;

		void restartGame();
		void forceRestart();
	};

	template <class ... Args>
	void Server::send(sf::IpAddress address, sf::Uint16 port, ServerCodes code, Args&&... args) {
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
		std::cout << "Отправка с порта: " << m_output.getLocalPort() << std::endl;
		m_launched.store(true);
	}

	inline void Server::startGame() {
		//Обновление переменных состояния
		m_state.game_ended.store(false);
		m_state.ready_testing.store(false);
		m_state.restart_request.store(false);
		m_state.game_started.store(true);
		//Сообщаем игрокам о начале игры
		for (const auto& bundle : m_players) {
			if (bundle.second.alive() && bundle.second.isReady())
				send(bundle.first, bundle.second.getPort(), ServerCodes::GAME_STARTED);
		}
		m_chrono.game_start = Chrono::clock::now();
	}

	inline void Server::endGame() {
		if (m_state.game_started) {
			for (const auto& bundle : m_players) {
				if (bundle.second.alive() && bundle.second.isReady())
					send(bundle.first, bundle.second.getPort(), ServerCodes::GAME_ENDED);
			}
			m_state.restart_request.store(false);
			m_state.force_request.store(false);
			m_state.game_started.store(false);
			m_state.game_ended.store(true);
		}
		else {
			m_state.ready_testing.store(false);
			m_state.restart_request.store(false);
			m_state.game_started.store(false);
			m_state.force_request.store(false);
		}
		
	}

	inline void Server::send(Packet& pack, sf::IpAddress address, sf::Uint16 port) {
		std::cout << "Send: '";
		for (int i = 0; i < pack.size(); ++i)
			std::cout << static_cast<size_t>(static_cast<byte*>(pack.data())[i]) << ' ';
		std::cout << "' to " << address << ":" << port << std::endl;
		
		m_output.send(pack.data(), pack.size(), address, port);
	}

	inline void GameState::set_default() {
		game_started	= false;
		game_ended		= false;
		restart_request = false;
		ready_testing	= false;
		force_request   = false;
	}

	inline GameState::GameState() {
		set_default();
	}

	inline bool Password::isValid(const char* pas) {
		for (int i = 0; i < LENGTH; ++i)
			if (password[i] != pas[i])
				return false;
		return true;
	}

	inline Password::Password(const char* pas) {
		std::memcpy(password, pas, TERMINATOR_LENGTH);
	}

	inline sf::IpAddress IPAlias::convert(sf::IpAddress ip) const {
		if (ip == mask_ip)
			return alias.load();
		return ip;
	}

	inline IPAlias::IPAlias(const sf::IpAddress& mask):
		mask_ip(mask), alias(mask){ 
	}

	inline Chrono::Chrono(crdelay kill, crdelay inactive, crdelay warning):
		kill_delay(kill), inactive_delay(inactive), warning_delay(warning) {
	}

	inline Server::Server(const char password[9], unsigned short port,
							Chrono::crdelay kill,
							Chrono::crdelay inactive,
							Chrono::crdelay warning):
		m_launched(false),
		m_input_thread(port, 255, 5),
		m_password(password),
		m_host(sf::IpAddress::LocalHost),
		m_chrono(kill, inactive, warning) {
	}

	inline void Server::onPause() {
		m_input_thread.pause();
	}

	inline void Server::onFrame() {
		void* received_memory = m_input_thread.get();

		if (m_state.force_request) {
			endGame();
			for (auto& player : m_players) {
				player.second.setDefaultState();
				player.second.ready();
				send(player.first, player.second.getPort(), ServerCodes::RESP_CHECK);
			}
			startGame();
		}

		if (m_state.restart_request) {
			endGame();
			m_state.ready_testing.store(true);

			for (auto& player : m_players) {
				player.second.setDefaultState();
				send(player.first, player.second.getPort(), ServerCodes::RESP_CHECK);
			}
		}
		
		if (received_memory != nullptr) {
			//Cчитывание пакета из буффера и создание вспомогательного объекта
			PacketPrefix prefix = *static_cast<PacketPrefix*>(received_memory);
			prefix.ip = m_host.convert(prefix.ip);
			
			received_memory = shift(received_memory, sizeof(PacketPrefix));
			Packet pack(received_memory, prefix.size);

			//Проверяем что в пакете есть код
			if (pack.enoughMemory<byte>()) {
				const ClientCodes code = static_cast<ClientCodes>(*pack.read<byte>());

				//Проверяем, что игрок зарегистрирован
				auto iterator = m_players.find(prefix.ip);
				if (iterator == m_players.end()) { //Игрок незарегистрирован
					std::cout << "Получен запрос от неизвестного пользователя: " << static_cast<byte>(code) << '\n';
					std::cout << '\t' << prefix.ip << std::endl;
					if ((code == ClientCodes::REGISTER)  
						&&  (!m_state.game_started)
						&&	(!m_state.ready_testing)
						&& (pack.enoughMemoryMany<sf::Uint16>(8u))) {
						//Чтение пакета
						const char* password			= pack.read<char>(8);
						const unsigned short send_port	= *pack.read<sf::Uint16>();
						const size_t size				= pack.availableSpace();
						//Если пароль верен
						if (m_password.isValid(password)) {
							std::string name(pack.read<char>(size), size);
							m_players.insert(iterator, std::make_pair(prefix.ip, Player(send_port, std::move(name))));
							send(prefix.ip, send_port, ServerCodes::REGISTER, prefix.ip);
						} else {
							std::cout << "Запрос регистрации с неверным паролем: " << static_cast<byte>(code) << '\n';
						}
					}
					else {
						std::cout << "Ошибка." << std::endl;
					}
				}
				else {
					auto& player = iterator->second;
					const auto& player_ip = iterator->first;
					
					player.updateLastRequest();
					
					switch (code) {
					case ClientCodes::REGISTER: //Перерегистрация
						std::cout << "Получен запрос: регистрация. От " << player.getName() << '\n';
						if ((!m_state.game_started)
							&& (!m_state.ready_testing)
							&& (pack.enoughMemoryMany<sf::Uint16>(8u))) {
							//Чтение пакета
							const char* password = pack.read<char>(8);
							const unsigned short send_port = *pack.read<sf::Uint16>();
							const size_t size = pack.availableSpace();
							//Если пароль верен
							if (m_password.isValid(password)) {
								std::string name(pack.read<char>(size), size);
								player.setPort(send_port);
								std::cout << "Смена имени: " << player.getName() << " -> " << name << std::endl;
								player.setName(std::move(name));
								send(player_ip, send_port, ServerCodes::REGISTER, player_ip);
							}
							else {
								std::cout << "Запрос регистрации с неверным паролем: " << static_cast<byte>(code) << '\n';
							}
						}
						else {
							std::cout << "Ошибка." << std::endl;
						}
						break;
					case ClientCodes::DELETE: //Удаление игрока.
						if (!m_state.game_started) {
								m_players.erase(iterator);
						}
						break;
					case ClientCodes::READY: //READY
						if (m_state.ready_testing) {
							player.ready();

							//Проверяем, что все игроки готовы
							bool start = true;
							for (const auto& bundle : m_players)
								if (bundle.second.alive() && !bundle.second.isReady()) {
									start = false;
									break;
								}

							if (start && !m_state.game_started)
								startGame();
						}
						break;
					case ClientCodes::DEATH: //Смерть
						if (m_state.game_started && player.alive()) {
							if (pack.enoughMemory<byte>(4)) {
								Packet packet(5);
								packet.write(static_cast<byte>(ServerCodes::DEATH));
								packet.write(player_ip);
								
								int alive_counter = 0;
								for (const auto& bundle : m_players) {
									if (bundle.second.alive() && bundle.second.isReady()) {
										send(packet, bundle.first, bundle.second.getPort());
										++alive_counter;
									}
								}
								if (alive_counter < 2) {
									endGame();
								}

								const byte* raw_ip = pack.read<byte>(4);
								sf::IpAddress killer(raw_ip[0], raw_ip[1], raw_ip[2], raw_ip[3]);

								DEMONORIUM_SIMPLE_FIND(m_players, find, killer, killer_player) {
									killer_player->second.incKillCounter();
								}
								else {
									killer = player_ip;
								}
								player.kill(killer);
								player.acceptKill();
							}
						}
						break;
					case ClientCodes::TABLE: //Запрос таблицы
					{
						if (m_state.game_started && player.alive() && player.isReady()) {
							size_t count = 252 / 4;
							byte fcount = 0;

							Packet packet(255);

							packet.write(byte(ServerCodes::TABLE));
							packet.write(byte(0));

							for (const auto& bundle : m_players) {
								if (bundle.second.alive() && bundle.second.isReady()) {
									packet.write(bundle.first);
									++fcount;
								}
							}
							static_cast<byte*>(packet.data())[1] = fcount;
							send(packet, player_ip, player.getPort());
						}
					}
					break;

					case ClientCodes::ACTIVE: //Проверка активности
					{
						if (player.alive())
							player.reset_death();
					}
					break;
					case ClientCodes::NAME: //Запрос внешнего ip
					{
						send(player_ip, player.getPort(), ServerCodes::REGISTER, player_ip);
					}
					break;
					case ClientCodes::KILL: //Запрос на убийство
					{
						if (m_state.game_started && player.alive() && player.isReady()) {
							if (pack.enoughMemory<byte>(4)) {
								const byte* raw_ip = pack.read<byte>(4);
								const sf::IpAddress killed(raw_ip[0], raw_ip[1], raw_ip[2], raw_ip[3]);

								auto killed_player = m_players.find(killed);
								if (killed_player != m_players.end()) {
									killed_player->second.kill(player_ip);
									send(player_ip, player.getPort(), ServerCodes::RESP_CHECK);
								} else {
									std::cout << "Unknown killed player";
								}
							}
						}
					}
					break;
					
					default:
						std::cerr << "Unknown code from " << prefix.ip.toString() << "\n";
					}
				}	
			}
		}

		//Игра в режиме опрашивания
		if (m_state.ready_testing) {
			for (auto& bundle : m_players) {
				auto& player = bundle.second;
				auto current_time = Chrono::clock::now();
				auto dt = current_time - player.getLastRequest();
				//Переодически опрашиваем всех игроков
				if (dt > m_chrono.warning_delay) {
					send(bundle.first, player.getPort(), ServerCodes::READY_REQ);
				}
			}
		} else if (m_state.game_started) {
			//Если игра началась периодически проверяем, что игроки живы 
			for (auto& player_p: m_players) {
				auto& player = player_p.second;
				if (player.alive() && player.isReady()) {
					auto current_time = Chrono::clock::now().time_since_epoch();
					auto dt = std::chrono::duration_cast<Chrono::delay>(current_time - player.getLastRequest().time_since_epoch()).count();
					if (player.on_death()) {
						if (dt > m_chrono.kill_delay.count()) {
							player.acceptKill();
							auto killer = m_players.find(player.getKillerIP());
							killer->second.incKillCounter();

							Packet packet(5);
							packet.write(static_cast<byte>(ServerCodes::DEATH));
							packet.write(player_p.first);

							int alive_counter = 0;
							for (const auto& bundle : m_players) {
								if (bundle.second.alive() && bundle.second.isReady()) {
									send(packet, bundle.first, bundle.second.getPort());
									++alive_counter;
								}
							}
							if (alive_counter < 2) {
								endGame();
							}
						}
						else if (dt > m_chrono.warning_delay.count()) {
							auto dt2 = std::chrono::duration_cast<Chrono::delay>(current_time - player.getLastWarning().time_since_epoch()).count();
							if (dt2 > m_chrono.warning_delay.count()) {
								player.updateLastWarning();
								send(player_p.first, player_p.second.getPort(), ServerCodes::RESP_CHECK);
							}
						}
					}
					else if (dt > m_chrono.kill_delay.count()) {
						player.kill(player_p.first);
						player.acceptKill();

						Packet packet(5);
						packet.write(static_cast<byte>(ServerCodes::DEATH));
						packet.write(player_p.first);

						int alive_counter = 0;
						for (const auto& bundle : m_players) {
							if (bundle.second.alive() && bundle.second.isReady()) {
								send(packet, bundle.first, bundle.second.getPort());
								++alive_counter;
							}
						}
						if (alive_counter < 2) {
							endGame();
						}
					}
					else if (dt > m_chrono.warning_delay.count()) {
						auto dt2 = std::chrono::duration_cast<Chrono::delay>(current_time - player.getLastWarning().time_since_epoch()).count();
						if (dt2 > m_chrono.warning_delay.count()) {
							send(player_p.first, player_p.second.getPort(), ServerCodes::RESP_CHECK);
							player.updateLastWarning();
						}
					} 
				}
			}
		}
	}

	inline void Server::onUnPause() {
		m_input_thread.run();
	}

	inline void Server::onDestruction() {
		endGame();
		
		for (auto& player : m_players) {
			player.second.setDefaultState();
		}
	}

	inline void Server::restartGame() {
		m_state.restart_request = true;
	}

	inline void Server::forceRestart() {
		m_state.force_request = true;
	}
}

#pragma once

#include <set>

#include "BaseThread.h"
#include "InputThread.h"
#include "Player.h"

#include "Packet.h"
#include "Log.h"


#include <DSFML/Aliases.h>


DEMONORIUM_ALIASES;
DEMONORIUM_LOCAL_USE(demonorium::memory::memory_declarations);

namespace demonorium
{
	class Server;


	struct GameState {
		//���� � ��������
		bool game_started;
		//����� ���������� �� ����������
		bool ready_testing;
		//���� ������� �����������
		bool game_ended;
		
		void set_default();
		GameState();
	};

	//������ ���� ������� � ��������� ������ �� ����������� �����
	struct Password {
		static constexpr const size_t LENGTH = 8;
		static constexpr size_t TERMINATOR_LENGTH = LENGTH + 1;
		char password[TERMINATOR_LENGTH];
		
		bool isValid(const char* string);

		Password(const char* string);
	};

	//��������� mask_ip �� ��������� � alias
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
		
		//�������� ����� ��������� kill ����� ����������������� �������
		const delay kill_delay;
		//�������� ����� ������� ����� ����������� �������������
		const delay inactive_delay;
		//�������� ����� ��������������� � �����������
		const delay warning_delay;
		//����� ������ ����
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

	enum class UserRequest: byte {
		START_GAME	 = 0,
		FORCE_START	 = 1,
		CLEAR		 = 2,
		FORCE_ESTART = 3,
		END_GAME	 = 4
	};
	
	class Server : public BaseThread {
		friend class ServerAPI;

		//��������� ��� ��������� �� �����-������� �� ��������� ������
		using ServerResponse = void (Server::*)(const sf::IpAddress& IP, Player& player, Packet& packet);
		//��������� ��� ��������� �� �����-������� �� ������ �� ��������� 
		using UserResponse = void (Server::*)();
		
		//��������, ��� ������ ������� �������
		std::atomic<bool> m_launched;
		
		InputThread		m_input_thread;
		GameState		m_state;
		Password		m_password;
		IPAlias			m_host;
		Chrono			m_chrono;
		sf::UdpSocket	m_output;

		Log m_log;

		using PlayerMap = std::map<sf::IpAddress, Player>;
		using PlayerBundle = PlayerMap::iterator;
		PlayerMap m_players;
		

		std::unordered_map<byte, ServerResponse> m_server_response;
		std::unordered_map<byte, UserResponse>	 m_user_response;
		TwoPageInput m_requests;
		
		//����������� ������� ������
		void registerPlayer(std::map<sf::IpAddress, Player>::iterator& hint, const sf::IpAddress& IP, Packet& packet);
		//�������� ���� ������� �� �������
		void removeByCondition(std::function<bool(PlayerBundle& it)> deleter, std::string message);
		
		void updatePlayer(	const sf::IpAddress& IP, Player& player, Packet& packet);
		void removePlayer(	const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerReady(	const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerDie(		const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerReqTable(const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerResponse(const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerReqName(	const sf::IpAddress& IP, Player& player, Packet& packet);
		void playerKill(	const sf::IpAddress& IP, Player& player, Packet& packet);
		
		void requestStart();
		void requestForce();
		void requestClear();
		void requestForceExists();
		void requestEndGame();

		//��������� ���� ������� ServerCodes::READY_REQ
		void sendReadyRequest(const Chrono::time_point& current_time);
		//��������� ����������
		void checkLife(const Chrono::time_point& current_time);
		//�������� ������ ���� � �������� �������
		void startGame();
		//��������� ���� � �������� �������
		void endGame();
		
		//��������� ������ �� ip � port 
		template<class ... Args>
		void response(sf::IpAddress address, sf::Uint16 port, ServerCodes code, Args&& ...);
		template<class T, class ... Args>
		void response(Packet& pack, sf::IpAddress address, sf::Uint16 port, const T& a, Args&& ... args);
		void response(Packet& pack, sf::IpAddress address, sf::Uint16 port);
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

		void request(UserRequest request);
	};

	template <class ... Args>
	void Server::response(sf::IpAddress address, sf::Uint16 port, ServerCodes code, Args&&... args) {
		Packet packet(255);
		response(packet, address, port, code, std::forward<Args>(args)...);
	}

	template <class T, class ... Args>
	void Server::response(Packet& pack, sf::IpAddress address, sf::Uint16 port, const T& a, Args&&... args) {
		pack.write(a);
		response(pack, address, port, std::forward<Args>(args)...);
	}
	
	void Server::onInit() {
		m_log.open("Server.log");
		m_input_thread.start();
		m_output.bind(sf::Socket::AnyPort);
		m_log.write("������ ���� ��� ��������: ", m_output.getLocalPort());

		m_players.clear();
		
		
		m_log.write_important("������ �������!");	
		m_launched.store(true);
	}

	inline void Server::registerPlayer(std::map<sf::IpAddress, Player>::iterator& hint, const sf::IpAddress& IP, Packet& packet) {
		//�������� ��������� � ������� ������
		m_log.write("������������� ������� �����������: ", IP.toString());
		if (!m_state.game_started && !m_state.ready_testing) {
			if (packet.enoughMemoryMany<sf::Uint16>(Password::LENGTH)) {
				//������ ������
				const char* password = packet.read<char>(Password::LENGTH);

				m_log.write("������: ", std::string(password, Password::LENGTH));
				//���� ������ �����
				if (m_password.isValid(password)) {
					const unsigned short send_port = *packet.read<sf::Uint16>();
					const size_t size = packet.availableSpace();

					m_log.write("����: ", send_port);
					
					std::string name(packet.read<char>(size), size);
					m_log.write("���: ", name);

					//��������� ������ � ������ �������
					m_players.insert(hint, std::make_pair(IP, Player(send_port, std::move(name), IP)));
					m_log.write_important("�������� �����������!");

					response(IP, send_port, ServerCodes::REGISTER, IP);
					m_log.write("����������� � �����������: ", name, " : ", IP.toString());
				}
				else {
					m_log.write("������ �������: �������� ������");
				}
			}
			else {
				m_log.write("������ �������: ������������� ������ �������");
			}
		} else {
			m_log.write("������ �������: �������� ��������� ����");
		}
	}

	inline void Server::removeByCondition(std::function<bool(PlayerBundle& it)> deleter,
			std::string message) {

		bool found;
		do {
			found = false;
			auto it = m_players.begin();
			for (; it != m_players.end(); ++it) {
				if (deleter(it)) {
					found = true;
					break;
				}
			}
			if (found) {
				m_log.write("����� ", it->second.getName(), " ����� �����. �������: ", message);
				m_players.erase(it);
			}
		} while (found);
	}

	inline void Server::updatePlayer(const sf::IpAddress& IP, Player& player, Packet& packet) {
		//�������� ��������� � ������� ������
		m_log.write(player.getName(), ": ������ ����������");
		if (!m_state.game_started && !m_state.ready_testing) {
			if (packet.enoughMemoryMany<sf::Uint16>(Password::LENGTH)) {
				//������ ������
				const char* password = packet.read<char>(Password::LENGTH);

				m_log.write("������: ", std::string(password, Password::LENGTH));
				//���� ������ �����
				if (m_password.isValid(password)) {
					const unsigned short send_port = *packet.read<sf::Uint16>();
					const size_t size = packet.availableSpace();

					m_log.write("����� ����: ", send_port);

					std::string name(packet.read<char>(size), size);
					m_log.write("����� ���: ", name);

					//��������� ������ � ������ �������
					player.setName(std::move(name));
					player.setPort(send_port);
					m_log.write_important("�������� ����������!");

					response(IP, send_port, ServerCodes::REGISTER, IP);
					m_log.write("����������� �� ����������: ", name, " : ", IP.toString());

					player.resurrection();
				}
				else {
					m_log.write("������ �������: �������� ������");
				}
			}
			else {
				m_log.write("������ �������: ������������� ������ �������");
			}
		}
		else {
			m_log.write("������ �������: �������� ��������� ����");
		}
	}

	inline void Server::removePlayer(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ ��������");
		//��������� ����� ������, ���� ���� �� �������� ��� ����� �� ��� ��������
		if (!m_state.game_started || !player.isReady()) {
			m_log.write_important("�������� ������: ", player.getName());
			m_players.erase(m_players.find(IP));
		} else {
			m_log.write("������ �������: �������� ��������� ����");
		} 
	}

	inline void Server::playerReady(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ ����������");
		//������� �� ����������� ����������� ������ ���� ��� ����� �������
		if (m_state.ready_testing) {
			player.ready();
			m_log.write(player.getName(), ": ����� ����� � ����");
			
			//���������, ��� ��� ������ ������
			bool start = true;
			for (const auto& bundle : m_players)
				if (bundle.second.alive() && !bundle.second.isReady()) {
					start = false;
					break;
				}
			
			//���� ��� ������ ������ - �������� ����
			if (start) {
				m_log.write("��� ������ ������");
				startGame();
			}
		}
		else {
			m_log.write("������ �������: �������� ��������� ����");
		}
	}

	inline void Server::playerDie(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ ������");
		if (m_state.game_started && player.alive() && player.isReady()) {
			if (packet.enoughMemory<byte>(4)) {
				Packet packet(5);
				packet.write(static_cast<byte>(ServerCodes::DEATH));
				packet.write(IP);
									
				int alive_counter = 0;
				for (const auto& bundle : m_players) {
					if (bundle.second.alive() && bundle.second.isReady()) {
						m_log.write("����������� � ������: ", bundle.second.getName(), " : ", bundle.first.toString());
						response(packet, bundle.first, bundle.second.getPort());
						++alive_counter;
					}
				}
				if (alive_counter < 2) {
					m_log.write("�������� ����� 2 ����� �������.");
					endGame();
				}

				const byte* raw_ip = packet.read<byte>(4);
				sf::IpAddress killer(raw_ip[0], raw_ip[1], raw_ip[2], raw_ip[3]);

				DEMONORIUM_SIMPLE_FIND(m_players, find, killer, killer_player) {
					m_log.write("����� ", player.getName(), " ���� ������� ", killer_player->second.getName());
					killer_player->second.incKillCounter();
				}
				else {
					m_log.write("����������� ������");
					killer = IP;
				}
				if (killer == IP)
					m_log.write_important("������������: ", killer.toString());
				
				m_log.write("�������� ������");
				player.kill(killer);
				player.acceptKill();
			}
			else {
				m_log.write("������ �������: ������������� ������ �������");
			}
		}
		else {
			m_log.write("������ �������: �������� ��������� ���� ��� ����� ��� ����/�� �����");
		}
	}

	inline void Server::playerReqTable(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ �������");
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
			response(packet, IP, player.getPort());
		}
		else {
			m_log.write("������ �������: �������� ��������� ���� ��� ����� ��� ����/�� �����");
		}
	}

	inline void Server::playerResponse(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ ������������� �����");
		if (player.alive() && player.isReady())
			player.reset_death();
		else
			m_log.write("������ �������: ����� ��� ���� ��� �� �����");
	}

	inline void Server::playerReqName(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ �������� �����");
		response(IP, player.getPort(), ServerCodes::REGISTER, IP);
	}

	inline void Server::playerKill(const sf::IpAddress& IP, Player& player, Packet& packet) {
		m_log.write(player.getName(), ": ������ ��������");
		if (m_state.game_started && player.alive() && player.isReady()) {
			if (packet.enoughMemory<byte>(4)) {
				const byte* raw_ip = packet.read<byte>(4);
				const sf::IpAddress killed(raw_ip[0], raw_ip[1], raw_ip[2], raw_ip[3]);

				DEMONORIUM_SIMPLE_FIND(m_players, find, killed, killed_player) {
					if (killed_player->second.isReady() && killed_player->second.alive()) {
						killed_player->second.kill(IP);
						m_log.write("������ �������� ����: ", killed_player->second.getName(), " : ", killed);
						response(killed, killed_player->second.getPort(), ServerCodes::RESP_CHECK);
						m_log.write("����������� � ������������: ", killed_player->second.getName(), " : ", killed_player->first.toString());
					} else {
						m_log.write("������ �������: ���� �� ������ � ���� ��� ��� ������");
					}
				}
				else {
					m_log.write("������ �������: ����������� ����: ", killed.toString());
				}
			} else {
				m_log.write("������ �������: ������������� ������ �������");
			}
		}
		else {
			m_log.write("������ �������: �������� ��������� ���� ��� ����� ��� ����/�� �����");
		}
	}

	inline void Server::requestStart() {
		m_log.write_important("�������������: ������ ������ ����");
		endGame();
		if (m_state.game_ended) {
			m_state.game_ended = false;
			m_log.write("���������� ���������� ����, �������������� �������");
			removeByCondition([](PlayerBundle& it) { return !it->second.alive(); }, "����� ���� �� ����� �����������");
		}
		m_state.ready_testing = true;

		for (auto& player : m_players) {
			player.second.setDefaultState();
			response(player.first, player.second.getPort(), ServerCodes::READY_REQ);
			m_log.write("������ ����������: ", player.second.getName(), " : ", player.first.toString());
		}
	}

	inline void Server::requestForce() {
		m_log.write_important("�������������: ������ ��������������� ������ ����");

		endGame();
		if (m_state.game_ended) {
			m_state.game_ended = false;
			m_log.write("���������� ���������� ����, �������������� �������");
			removeByCondition([](PlayerBundle& it) { return !it->second.alive(); }, "����� ���� �� ����� �����������");
		}
		
		for (auto& player : m_players) {
			player.second.setDefaultState();
			player.second.ready();
			response(player.first, player.second.getPort(), ServerCodes::READY_REQ);
			m_log.write("������ ����������: ", player.second.getName(), " : ", player.first.toString());			
		}
		
		startGame();
	}

	inline void Server::requestClear() {
		m_log.write_important("�������������: ����� ��������� ���� � ������ �������");
		m_players.clear();
		m_state.set_default();
	}

	inline void Server::requestForceExists() {
		m_log.write_important("�������������: ������ ��������������� ������ ����");

		if (m_state.ready_testing) {
			startGame();
		}
		else {
			endGame();
			if (m_state.game_ended) {
				m_state.game_ended = false;
				m_log.write("���������� ���������� ����, �������������� �������");
				removeByCondition([](PlayerBundle& it) { return !it->second.alive(); }, "����� ���� �� ����� �����������");
			}


			for (auto& player : m_players) {
				player.second.setDefaultState();
				player.second.ready();
				response(player.first, player.second.getPort(), ServerCodes::READY_REQ);
				m_log.write("������ ����������: ", player.second.getName(), " : ", player.first.toString());
			}
		}
		
		startGame();
	}

	inline void Server::requestEndGame() {
		m_log.write_important("�������������: ������ ��������� ����");
		endGame();
	}

	inline void Server::sendReadyRequest(const Chrono::time_point& current_time) {
		for (auto& bundle : m_players) {
			auto& player = bundle.second;
			if (!player.isReady()) {
				auto dt = current_time - player.getLastWarning();
				//������������ ���������� ���� �������
				if (dt > m_chrono.warning_delay) {
					player.updateLastWarning();
					response(bundle.first, player.getPort(), ServerCodes::READY_REQ);
					m_log.write("������ ����������: ", player.getName(), " : ", bundle.first.toString());
				}
			}
		}
	}

	inline void Server::checkLife(const Chrono::time_point& current_time) {
		for (auto& player_p : m_players) {
			auto& player = player_p.second;
			if (player.alive() && player.isReady()) {
				auto dt = std::chrono::duration_cast<Chrono::delay>(current_time - player.getLastRequest()).count();
				
				if (dt > m_chrono.kill_delay.count() && player.on_death() || dt > m_chrono.inactive_delay.count()) {
					if (player.getKillerIP() == sf::IpAddress(0, 0, 0, 0)) {
						m_log.write("����� ", player.getName(), " ����� ��-�� ���������� ����������");
						player.kill(player_p.first);
					} else {
						auto killer = m_players.find(player.getKillerIP());
						killer->second.incKillCounter();
						
						m_log.write("����� ", player.getName(), " ���� ������� ", killer->second.getName(), " � ������� ������� �� ��������");
					}
					player.acceptKill();

					Packet packet(5);
					packet.write(static_cast<byte>(ServerCodes::DEATH));
					packet.write(player_p.first);

					int alive_counter = 0;
					for (const auto& bundle : m_players) {
						if (bundle.second.alive() && bundle.second.isReady()) {
							response(packet, bundle.first, bundle.second.getPort());
							m_log.write("����������� � ������: ", bundle.second.getName(), " : ", bundle.first.toString());
							++alive_counter;
						}
					}
					if (alive_counter < 2) {
						endGame();
					}
				}
				else if (dt > m_chrono.warning_delay.count()) {
					auto dt2 = std::chrono::duration_cast<Chrono::delay>(current_time - player.getLastWarning()).count();
					if (dt2 > m_chrono.warning_delay.count()) {
						response(player_p.first, player_p.second.getPort(), ServerCodes::RESP_CHECK);
						player.updateLastWarning();
						m_log.write("����������� � ������������: ", player.getName(), " : ", player_p.first.toString());
					}
				}	
			}
		}
	}

	inline void Server::startGame() {
		//���������� ���������� ���������
		m_state.game_ended		= false;
		m_state.ready_testing	= false;
		m_state.game_started	= true;
		m_log.write_important("���� ��������!");
		//�������� ������� � ������ ����
		for (const auto& bundle : m_players) {
			if (bundle.second.alive() && bundle.second.isReady()) {
				response(bundle.first, bundle.second.getPort(), ServerCodes::GAME_STARTED);
				m_log.write("����������� � ������ ����: ", bundle.second.getName(), " : ", bundle.first.toString());
			}
		}
		m_chrono.game_start = Chrono::clock::now();
	}

	inline void Server::endGame() {
		if (m_state.game_started) {
			m_log.write_important("���� ���������!");
			for (const auto& bundle : m_players) {
				if (bundle.second.alive() && bundle.second.isReady()) {
					response(bundle.first, bundle.second.getPort(), ServerCodes::GAME_ENDED);
					m_log.write("����������� � ����� ����: ", bundle.second.getName(), " : ", bundle.first.toString());
				}
			}
			m_state.ready_testing = false;
			m_state.game_started  = false;
			m_state.game_ended    = true;
		}
		else {
			m_log.write("C���� ��������� ����");
			m_state.set_default();
		}
	}

	inline void Server::response(Packet& pack, sf::IpAddress address, sf::Uint16 port) {
		m_output.send(pack.data(), pack.size(), address, port);
	}

	inline void GameState::set_default() {
		game_started	= false;
		game_ended		= false;
		ready_testing	= false;
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
		m_input_thread(port, 255, 128),
		m_password(password),
		m_host(sf::IpAddress::LocalHost),
		m_chrono(kill, inactive, warning),
		m_log(true),
		m_requests(sizeof(ServerResponse), 32),
		m_server_response(std::numeric_limits<byte>::max()),
		m_user_response(std::numeric_limits<byte>::max()) {
		
		m_server_response[static_cast<byte>(ClientCodes::REGISTER)]  = &Server::updatePlayer;
		m_server_response[static_cast<byte>(ClientCodes::DELETE)]	 = &Server::removePlayer;
		m_server_response[static_cast<byte>(ClientCodes::DEATH)]	 = &Server::playerDie;
		m_server_response[static_cast<byte>(ClientCodes::KILL)]		 = &Server::playerKill;
		m_server_response[static_cast<byte>(ClientCodes::READY)]	 = &Server::playerReady;
		m_server_response[static_cast<byte>(ClientCodes::ACTIVE)]	 = &Server::playerResponse;
		m_server_response[static_cast<byte>(ClientCodes::NAME)]		 = &Server::playerReqName;
		m_server_response[static_cast<byte>(ClientCodes::TABLE)]	 = &Server::playerReqTable;

		m_user_response[static_cast<byte>(UserRequest::START_GAME)]   = &Server::requestStart;
		m_user_response[static_cast<byte>(UserRequest::FORCE_START)]  = &Server::requestForce;
		m_user_response[static_cast<byte>(UserRequest::FORCE_ESTART)] = &Server::requestForceExists;
		m_user_response[static_cast<byte>(UserRequest::CLEAR)]		  = &Server::requestClear;
		m_user_response[static_cast<byte>(UserRequest::END_GAME)]	  = &Server::requestEndGame;
	}

	inline void Server::onPause() {
		m_input_thread.pause();
	}

	inline void Server::onFrame() {
		//������� �� ����������
		void* request_memory = m_requests.read();
		
		if (request_memory != nullptr) {
			DEMONORIUM_SIMPLE_FIND(m_user_response, find, as_reference<byte>(request_memory), request) {
				std::mem_fn(request->second)(this);
			}
		}

		//������ �� ����
		void* received_memory = m_input_thread.get();
		if (received_memory != nullptr) {
			//C��������� ������ �� ������� � �������� ���������������� �������
			PacketPrefix prefix = as_reference<PacketPrefix>(received_memory);
			prefix.ip = m_host.convert(prefix.ip);
			
			received_memory = shift(received_memory, sizeof(PacketPrefix));
			Packet pack(received_memory, prefix.size);

			//��������� ��� � ������ ���� ���
			if (pack.enoughMemory<byte>()) {
				const auto code = static_cast<ClientCodes>(*pack.read<byte>());
				
				DEMONORIUM_SIMPLE_FIND(m_players, find, prefix.ip, sender) {
					sender->second.updateLastRequest();
					
					DEMONORIUM_SIMPLE_FIND(m_server_response, find, (byte)code, response) {
						std::mem_fn(response->second)(this, sender->first, sender->second, pack);
					} else {
						m_log.write(sender->second.getName(), ": ����������� ��� �������: ", static_cast<int>(code));
						m_log.write("����������: ", BinaryOutput(pack.data(), pack.size()));
					}
				} else {
					if (code == ClientCodes::REGISTER)
						registerPlayer(sender, prefix.ip, pack);
					else {
						m_log.write("������������ �����: ", prefix.ip);
					}
				}
			}
		}

		//������� �����
		auto current_time = Chrono::clock::now();
		
		
		if (m_state.ready_testing) {
			//���� ���� � ������ �����������, ����������� �������� ������� ����������
			sendReadyRequest(current_time);
		} else if (m_state.game_started) { 
			//���� ���� �������� ������������ ���������, ��� ������ ���� 
			checkLife(current_time);
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
	
	inline void Server::request(UserRequest request) {
		auto* request_memory = m_requests.write();
		std::memcpy(request_memory, &request, sizeof(UserRequest));
		m_requests.validWrite();
	}
}

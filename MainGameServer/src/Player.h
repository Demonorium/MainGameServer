#pragma once

#include <chrono>
#include <SFML/Network.hpp>


namespace demonorium
{
	namespace chrono
	{
		using namespace std::chrono;
		using namespace std::chrono_literals;
	}

	struct PlayerTimeInfo {
		using clock = std::chrono::system_clock;
		using point = clock::time_point;
		using delay = clock::duration;
		
		point last_request;
		point last_warning;
		point die_time;

		void set_default();
		PlayerTimeInfo();
	};


	struct Life {
		sf::IpAddress	killer;
		bool			alive;
		bool			ready;
		bool			killed;
		
		void set_default();
		Life();
	};
	
	class Player {
		std::string		m_name;
		PlayerTimeInfo	m_time;
		Life			m_life;
		sf::Uint16		m_port;
		size_t			m_kill_count;
	public:
		Player(sf::Uint16 port, std::string name);
		~Player() = default;

		Player(const Player&) = default;
		Player& operator =(const Player&) = default;

		//Игрок готов к началу игры
		bool isReady() const;
		//Ставит игроку флаг готовности
		void ready();
		//Сбросить состояние игрока на начало игры
		void setDefaultState();

		//Добавить 1 к счётчику убийств
		void incKillCounter();
		//Счётчик убийств
		auto getKillCounter() const;
		//IP убившего
		const auto& getKillerIP() const;
		
		void setName(std::string newName);
		void setPort(sf::Uint16 port);

		sf::Uint16 getPort() const;
		const std::string& getName() const;
		
		const chrono::system_clock::time_point& getDieTime() const;
		void kill(sf::IpAddress kiAddress);
		void acceptKill();
		bool alive() const;
		bool on_death() const;
		void reset_death();
			 
		void updateLastRequest();
		PlayerTimeInfo::point getLastRequest() const;

		void updateLastWarning();
		PlayerTimeInfo::point getLastWarning() const;
	};


	inline void PlayerTimeInfo::set_default() {
		last_request = clock::now();
		last_warning = last_request;
		die_time = point();
	}

	inline PlayerTimeInfo::PlayerTimeInfo() {
		set_default();
	}

	inline void Life::set_default() {
		killer = sf::IpAddress(0, 0, 0, 0);
		alive = true;
		ready = false;
		killed = false;
	}

	inline Life::Life() {
		set_default();
	}

	inline Player::Player(sf::Uint16 port, std::string name):
		m_port(port), m_name(std::move(name)), m_kill_count(0) {
	}

	inline void Player::setDefaultState() {
		m_time.set_default();
		m_life.set_default();
		m_kill_count = 0;
	}

	inline void Player::incKillCounter() {
		++m_kill_count;
	}

	inline auto Player::getKillCounter() const {
		return m_kill_count;
	}

	inline const auto& Player::getKillerIP() const {
		return m_life.killer;
	}

	inline void Player::setName(std::string newName) {
		m_name = std::move(newName);
	}

	inline void Player::setPort(sf::Uint16 port) {
		m_port = port;
	}

	inline sf::Uint16 Player::getPort() const {
		return m_port;
	}

	inline const std::string& Player::getName() const {
		return m_name;
	}

	inline const chrono::system_clock::time_point& Player::getDieTime() const {
		return m_time.die_time;
	}

	inline void Player::kill(sf::IpAddress kiAddress) {
		m_time.die_time = PlayerTimeInfo::clock::now();
		m_life.killer = kiAddress;
	}

	inline void Player::acceptKill() {
		m_life.alive = false;
	}

	inline bool Player::alive() const {
		return m_life.alive;
	}

	inline bool Player::on_death() const {
		return m_life.killed;
	}

	inline void Player::reset_death()  {
		m_life.killed = false;
	}

	inline void Player::updateLastRequest() {
		m_time.last_request = PlayerTimeInfo::clock::now();
		updateLastWarning();
	}

	inline PlayerTimeInfo::point Player::getLastRequest() const {
		return m_time.last_request;
	}

	inline void Player::updateLastWarning() {
		m_time.last_warning = PlayerTimeInfo::clock::now();
	}

	inline PlayerTimeInfo::point Player::getLastWarning() const {
		return m_time.last_warning;
	}

	inline void Player::ready() {
		m_life.ready = true;
	}

	inline bool Player::isReady() const {
		return m_life.ready;
	}
}

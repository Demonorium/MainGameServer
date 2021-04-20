#pragma once

#include <chrono>
#include <SFML/Network.hpp>

#include "Log.h"


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
		Log				m_log;
	public:
		Player(sf::Uint16 port, std::string name, sf::IpAddress ip);
		~Player() = default;

		Player(Player&& other);
		Player& operator =(Player&&) = default;

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
		void resurrection();
		
		
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

	inline Player::Player(sf::Uint16 port, std::string name, sf::IpAddress logip):
		m_port(port), m_name(std::move(name)), m_kill_count(0) {
		m_log.open("player_" + logip.toString() + ".log");
		
		m_log.write_important("Создание сущности игрока с именем: ", name, "; port: ", port);
	}

	inline Player::Player(Player&& other) :
		m_log(std::move(other.m_log)),
		m_name(std::move(other.m_name)),
		m_port(std::move(other.m_port)),
		m_kill_count(std::move(other.m_kill_count)),
		m_life(other.m_life),
		m_time(other.m_time) {
	}

	inline void Player::setDefaultState() {
		m_time.set_default();
		m_life.set_default();
		m_kill_count = 0;

		m_log.write_important("Сброс внутренного состояния");
	}

	inline void Player::incKillCounter() {
		m_log.write("Регистрация убийства");
		++m_kill_count;
	}

	inline auto Player::getKillCounter() const {
		return m_kill_count;
	}

	inline const auto& Player::getKillerIP() const {
		return m_life.killer;
	}

	inline void Player::setName(std::string newName) {
		if (m_name != newName) {
			m_log.write("Смена имени: \"", m_name, "\" -> \"", newName, '"');
			m_name = std::move(newName);
		}
	}

	inline void Player::setPort(sf::Uint16 port) {
		if (m_port != port) {
			m_log.write("Смена порта: \"", m_port, "\" -> \"", port, '"');
			m_port = port;
		}
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
		m_log.write("Зафиксирована попытка убийства: игроком ", kiAddress.toString());

		m_time.die_time = PlayerTimeInfo::clock::now();
		m_life.killer = kiAddress;
		m_life.killed = true;
	}

	inline void Player::acceptKill() {
		m_log.write_important("СМЕРТЬ!");
		m_life.alive = false;
	}

	inline bool Player::alive() const {
		return m_life.alive;
	}

	inline bool Player::on_death() const {
		return m_life.killed;
	}

	inline void Player::reset_death() {
		m_log.write("Попытка убийства отменена");
		m_life.killed = false;
	}

	inline void Player::resurrection() {
		m_log.write("Воскрешение");
		m_life.alive = true;
	}

	inline void Player::updateLastRequest() {
		if (!on_death())
			updateLastWarning();
		m_time.last_request = PlayerTimeInfo::clock::now();
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
		m_log.write("Игрок готов к игре");
		m_life.ready = true;
	}

	inline bool Player::isReady() const {
		return m_life.ready;
	}
}

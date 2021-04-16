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
	
	class Player {
		std::string m_name;
		unsigned short m_port;
		chrono::system_clock::time_point m_die_time;
		sf::IpAddress m_ki_address;
		size_t m_kill_count;
		chrono::system_clock::time_point m_last_request;

		bool m_killed;
		bool m_ready;
	public:
		Player(unsigned short port, std::string name);
		~Player() = default;
		
	

		Player(const Player&) = default;
		Player& operator =(const Player&) = default;

		void gameReset();
		
		void incKillCounter();
		auto getKillCounter() const;
		const auto& getKillerIP() const;
		
		void setName(std::string newName);
		void setPort(unsigned short port);

		
		unsigned short getPort() const;
		const std::string& getName() const;
		
		const chrono::system_clock::time_point& getDieTime() const;
		void kill(sf::IpAddress kiAddress);
		void acceptKill();
		bool alive() const;
		bool on_death() const;
		void reset_death_timer();
			 
		
		void ready();
		void notReady();
		bool isReady() const;

		void setLastRequest(const chrono::system_clock::time_point& tp)  {
			m_last_request = tp;
		}
		
		chrono::system_clock::time_point getLastRequest() const {
			return m_last_request;
		}
	};

	inline Player::Player(unsigned short port, std::string name):
		m_port(port), m_name(std::move(name)), m_last_request(std::chrono::system_clock::now()),
		m_killed(false){
		gameReset();
	}

	inline void Player::gameReset() {
		m_die_time = chrono::system_clock::time_point();
		m_kill_count = 0;
		m_ready = false;
		m_killed = false;
	}

	inline void Player::incKillCounter() {
		++m_kill_count;
	}

	inline auto Player::getKillCounter() const {
		return m_kill_count;
	}

	inline const auto& Player::getKillerIP() const {
		return m_ki_address;
	}

	inline void Player::setName(std::string newName) {
		m_name = std::move(newName);
	}

	inline void Player::setPort(unsigned short port) {
		m_port = port;
	}

	inline unsigned short Player::getPort() const {
		return m_port;
	}

	inline const std::string& Player::getName() const {
		return m_name;
	}

	inline const chrono::system_clock::time_point& Player::getDieTime() const {
		return m_die_time;
	}

	inline void Player::kill(sf::IpAddress kiAddress) {
		m_die_time = chrono::system_clock::now();
		m_ki_address = std::move(kiAddress);
	}

	inline void Player::acceptKill() {
		m_killed = true;
	}

	inline bool Player::alive() const {
		return !m_killed;
	}

	inline bool Player::on_death() const {
		return m_die_time.time_since_epoch() == chrono::system_clock::duration(NULL);
	}

	inline void Player::reset_death_timer()  {
		m_die_time = chrono::system_clock::time_point();
	}

	inline void Player::ready() {
		m_ready = true;
	}

	inline void Player::notReady() {
		m_ready = false;
	}

	inline bool Player::isReady() const {
		return m_ready;
	}
}

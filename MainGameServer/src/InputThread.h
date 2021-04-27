#pragma once
#include <functional>
#include <iostream>


#include "BaseThread.h"
#include <SFML/Network.hpp>
#include <utility>

#include <DSFML/Aliases.h>


DEMONORIUM_ALIASES;
DEMONORIUM_LOCAL_USE(demonorium::memory::memory_declarations);



namespace demonorium
{
	/**
	 * \brief Двухстраничный синхронный буффер без блокировок, предназначенный для использования в качестве
	 * "почтового ящика", универсален, т.к. работает с "сырой" памятью
	 * Не поддерживает выравнивание памяти.
	 */
	class TwoPageInput {
		void* m_page1;
		void* m_page2;

		const size_t m_block_size;
		const byte m_block_count;
		byte m_output_position;
		std::atomic<byte> m_input_position;
		std::atomic<bool> m_page_free;
		bool m_page_fill;
		bool m_read;

		inline void* _read();
	public:
		TwoPageInput(size_t blockSize, byte blockCount);
		~TwoPageInput();
		
		/**
		 * \brief Считать объект из буффера.
		 * \return Указатель на память из которой можно читать
		 */
		void* read();
		
		/**
		 * \brief Получить память для записи, после завершения работы с блоком должен быть вызван метод validWrite
		 * \return Указатель на память в которой можно конструировать объекты
		 */
		void* write();


		/**
		 * \brief  Завершает ввод данных в буфер
		 */
		void validWrite();
		size_t getBlockSize() const;
	};

	using namespace std::chrono_literals;
	class DDOSDefence {
		std::map<sf::IpAddress, std::pair<std::chrono::system_clock::time_point, size_t>> m_chrono_defence;
		std::chrono::milliseconds m_defence_time;
		size_t m_encounter_limit;
		
	public:
		DDOSDefence(std::chrono::milliseconds defenceTime, size_t limitCounter);

		bool packet(sf::IpAddress address);
	};

	
	struct PacketPrefix {
		size_t size;
		sf::IpAddress ip;

		PacketPrefix(size_t size, const sf::IpAddress& ip)
			: size(size),
			  ip(ip) {
		}
	};


	class InputThread: public BaseThread {
		TwoPageInput	m_buffer;
		sf::UdpSocket	m_socket;
		DDOSDefence		m_defence;
		std::mutex		m_mutex;
		void*			m_memory;
		unsigned int	m_port;
	protected:
		void onInit() override;
		void onFrame() override;
	public:
		InputThread(unsigned short port, size_t packetSize, byte packetCount, size_t defencePacketCount = 6, std::chrono::milliseconds defenceDuration = 500ms);
		~InputThread() override = default;

		void setPort(sf::Uint16 port);
		unsigned short getPort() const;

		inline void* get();
	};



	inline void* TwoPageInput::_read() {
		void* temp;
		if (m_read) {
			temp = shift(m_page1,m_output_position * m_block_size);
		}
		else {
			temp = shift(m_page2, m_output_position * m_block_size);
		}
		++m_output_position;
		return temp;
	}

	inline TwoPageInput::TwoPageInput(size_t blockSize, byte blockCount):
		m_block_size(blockSize + sizeof(size_t)), m_block_count(blockCount),
		m_page1(std::malloc(blockSize*blockCount)),
		m_page2(std::malloc(blockSize*blockCount)),
		m_output_position(0), m_input_position(0),
		m_page_free(true), m_page_fill(false), m_read(false){
	}

	inline TwoPageInput::~TwoPageInput() {
		std::free(m_page1);
		std::free(m_page2);
	}

	inline void* TwoPageInput::read() {
		byte shift = m_input_position.load();
		if (m_page_free.load()) {
			if (m_output_position < shift)
				return _read();
			return nullptr;
		}

		if (m_output_position == m_block_count) {
			m_page_free.store(true);
			m_output_position = 0;
			m_read = !m_read;

			if (m_output_position < shift) 
				return _read();

			return nullptr;
		}
		
		return _read();
	}

	inline void* TwoPageInput::write() {
		const auto pos = m_input_position.load();
		if (pos == m_block_count) {
			if (m_page_free) {
				m_page_free.store(false);
				m_input_position.store(0);
				m_page_fill = !m_page_fill;
			}
			else
				return nullptr;
		}
		if (m_page_fill)
			return shift(m_page1, pos * m_block_size);
		return shift(m_page2, pos * m_block_size);
	}

	inline void TwoPageInput::validWrite() {
		++m_input_position;
	}

	inline size_t TwoPageInput::getBlockSize() const {
		return m_block_size;
	}

	inline DDOSDefence::DDOSDefence(std::chrono::milliseconds defenceTime, size_t limitCounter):
		m_defence_time(defenceTime), m_encounter_limit(limitCounter){
	}

	inline bool DDOSDefence::packet(sf::IpAddress address) {
		auto it = m_chrono_defence.find(address);
		if (it == m_chrono_defence.end()) {
			m_chrono_defence.insert(it, std::make_pair(address, std::make_pair(std::chrono::system_clock::now(), 1)));
			return true;
		}
		if ((std::chrono::system_clock::now() - it->second.first) > m_defence_time) {
			it->second.second = 1;
			return true;
		}
		return (it->second.second++) < m_encounter_limit;
	}

	inline void InputThread::onInit() {
		m_socket.setBlocking(false);
		m_socket.bind(m_port);
	}

	inline void InputThread::onFrame() {
		if (m_memory == nullptr)
			m_memory = m_buffer.write();
		
		if (m_memory != nullptr) {
			void* shifted = shift(m_memory, sizeof(PacketPrefix));

			sf::IpAddress address;
			size_t		  received;
			unsigned short port;
			sf::Socket::Status result = m_socket.receive(shifted, m_buffer.getBlockSize() - sizeof(PacketPrefix), received, address, port);;

			if ((result == sf::Socket::Done)) {
				if (m_defence.packet(address)) {
					new (m_memory) PacketPrefix(received, address);
					m_memory = nullptr;
					m_buffer.validWrite();
				}
			} else {
				if (result == sf::Socket::Error) {
					std::cerr << "Input error: " << "sender ip: " << address << "; sender port: " << port << std::endl;
				}
			}
		}
	}

	inline InputThread::InputThread(unsigned short port, size_t packetSize, byte packetCount, size_t defencePacketCount, std::chrono::milliseconds defenceDuration):
		m_defence(defenceDuration, defencePacketCount),
		m_buffer(packetSize + sizeof(PacketPrefix), packetCount),
		m_memory(nullptr), m_port(port) {
	}

	inline void InputThread::setPort(sf::Uint16 port) {
		if (isRunning())
			pause();
		while (!isRealyPaused() && containsThread());

		m_port = port;
		m_socket.bind(port);
		run();
	}

	inline void* InputThread::get() {
		return m_buffer.read();
	}

	inline unsigned short InputThread::getPort() const {
		return m_socket.getLocalPort();
	}
}

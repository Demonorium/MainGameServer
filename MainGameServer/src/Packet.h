#pragma once

#include <SFML/Network.hpp>
#include <assert.h>
#include <memory.h>
#include <DSFML\Aliases.h>

DEMONORIUM_ALIASES;
DEMONORIUM_LOCAL_USE(demonorium::memory::memory_declarations);


namespace demonorium
{
	/**
	 * \brief Класс предназначен для представления отправляемых и получаемых данных, умеет читать данные разных типов
	 * Имеется перегрузка для sf::IpAddress
	 */
	class Packet {
		void*	m_memory;
		bool	m_control;
		size_t	m_size;

		void*	m_io_pos;
		size_t	m_io_offset;
	public:
		//Конструирует пакет на основе данных, пакет содержит УКАЗАТЕЛЬ на эти данные И НЕУПРАВЛЯЕТ их жизнью
		Packet(void* received, size_t size);
		//Конструирует пакет с хранилищем данных заданного размера. Хранилище разрушается при разрушении пакета
		Packet(size_t size);
		~Packet();

		//Указатель на начало хранилища
		void* data();
		//Размер хранилища
		size_t rawSize();
		// Количество считанных или записанных данных
		size_t size() const;

		//Сброс состояния чтения/записи
		void resetIO();

		//Считать массив размера count
		template<class T>
		const T* read(size_t count = 1);

		//Считать сырую память размера len
		const void* read(size_t len = 0);

		//Записать адрес в порядке 1 2 3 4 байты
		void write(sf::IpAddress address);

		//Записать объект
		template<class T>
		void write(const T& object);

		//Записать массив объектов
		template<class T>
		void write(const T* object, size_t count = 1);

		//Достаточно ли памяти для размещения массива объектов
		template<class T>
		bool enoughMemory(size_t count = 1) const;

		//Достаточно ли памяти для размещения последовательности объектов.
		template<class T, class ... Args>
		bool enoughMemoryMany(size_t current = 0) const;

		//Свободное место в пакете
		size_t availableSpace() const;
	};


	template <class T>
	const T* Packet::read(size_t count) {
		assert(enoughMemory<T>(count));
		const T* reference = static_cast<T*>(m_io_pos);

		m_io_offset += count*sizeof(T);
		m_io_pos = shift(m_io_pos, count*sizeof(T));
		
		return reference;
	}

	template <class T>
	void Packet::write(const T& object) {
		write(&object);
	}

	
	template <class T>
	void Packet::write(const T* object, size_t count) {
		assert(enoughMemory<T>(count));
		m_io_offset += count * sizeof(T);
		std::memcpy(m_io_pos, object, sizeof(T) * count);
		m_io_pos = shift(m_io_pos, count * sizeof(T));
	}

	template <class T>
	bool Packet::enoughMemory(size_t count) const {
		return (m_io_offset + sizeof(T)*count) <= m_size;
	}

	namespace
	{
		template<class T, class ... Args>
		size_t sum(T a, Args ... args) {
			return a + sum(args...);
		}

		size_t sum() {
			return 0;
		}
	}
	
	template <class T, class ... Args>
	bool Packet::enoughMemoryMany(size_t current) const {
		return (current + sizeof(T) + sum(sizeof(Args) ...)) <= m_size;
	}



	inline Packet::Packet(void* received, size_t size):
		m_memory(received), m_control(false),
		m_size(size){
		resetIO();
	}

	inline Packet::Packet(size_t size):
		m_memory(std::malloc(size)), m_control(true),
		m_size(size){
		resetIO();
	}

	inline Packet::~Packet() {
		if (m_control)
			std::free(m_memory);
	}

	inline void* Packet::data() {
		return m_memory;
	}

	inline size_t Packet::rawSize() {
		return m_size;
	}

	inline size_t Packet::size() const {
		return m_io_offset;
	}

	// ReSharper disable once CppInconsistentNaming
	inline void Packet::resetIO() {
		m_io_offset = 0;
		m_io_pos = m_memory;
	}

	inline const void* Packet::read(size_t len) {
		assert((m_io_offset + len) <= m_size);
		const void* reference = m_io_pos;

		m_io_offset += len;
		m_io_pos = shift(m_io_pos, len);

		return reference;
	}
	namespace
	{
		template<size_t size>
		using byteArray = byte[size];
	}
	inline void Packet::write(sf::IpAddress address) {
		const auto toNumber = address.toInteger();
		const auto& arr           = reinterpret_cast<const byteArray<4>&>(toNumber);
		write(arr[3]);
		write(arr[2]);
		write(arr[1]);
		write(arr[0]);
	}

	inline size_t Packet::availableSpace() const {
		return m_size - m_io_offset;
	}
}

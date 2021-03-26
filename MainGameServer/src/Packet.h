#pragma once

#include <SFML/Network.hpp>
#include <assert.h>
#include <memory.h>


namespace demonorium
{	
	class Packet {
		void* m_memory;
		bool m_control;
		size_t m_size;

		void* m_io_pos;
		size_t m_io_offset;
	public:
		Packet(void* received, size_t size);
		Packet(size_t size);
		~Packet();
		
		void* data();
		size_t rawSize();
		size_t size() const;
		
		void resetIO();

		template<class T>
		const T* read(size_t count = 1);

		const void* read(size_t len = 0);
		
		template<class T>
		void write(const T& object);

		template<class T>
		void write(const T* object, size_t count = 1);
		
		template<class T>
		bool enoughMemory(size_t count = 1) const;

		template<class T, class ... Args>
		bool enoughMemoryMany(size_t current = 0) const;

		size_t availableSpace() const;
	};


	template <class T>
	const T* Packet::read(size_t count) {
		assert(enoughMemory<T>(count));
		const T* reference = static_cast<T*>(m_io_pos);

		m_io_offset += count*sizeof(T);
		m_io_pos = reinterpret_cast<void*>(reinterpret_cast<size_t>(m_io_pos) + count*sizeof(T));
		
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
		std::memcpy(m_io_pos, &object, sizeof(T) * count);
		m_memory =  reinterpret_cast<void*>(reinterpret_cast<size_t>(m_io_pos) + count * sizeof(T));
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
		return (current + sizeof(T) + sum(sizeof(Args) ...));
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
		m_io_pos = reinterpret_cast<void*>(reinterpret_cast<size_t>(m_io_pos) + len);

		return reference;
	}

	inline size_t Packet::availableSpace() const {
		return m_size - m_io_offset;
	}
}

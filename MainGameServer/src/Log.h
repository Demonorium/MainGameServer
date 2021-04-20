#pragma once

#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>
#include <DSFML/Aliases.h>

namespace demonorium
{

	class BinaryOutput {
		friend std::ostream& operator <<(std::ostream& stream, const BinaryOutput& output);
		
		const void* m_pointer;
		size_t m_size;
	public:
		BinaryOutput(const void* pointer, size_t size);
	};


	inline std::ostream& operator<<(std::ostream& stream, const BinaryOutput& output) {
		for (int i = 0; i < output.m_size; ++i) {
			uint byte = static_cast<uint>(reinterpret_cast<aliases::uint8*>(output.m_size)[i]);
			if (byte < 10) {
				stream << byte << "   ";
			}
			else if (byte < 100) {
				stream << byte << "  ";
			}
			else {
				stream << byte << ' ';
			}
		}
		return stream;
	}


	class Log {
		std::fstream m_output;
		bool m_console;
		
		template<class T, class ... Args>
		void _log(const T& obj, Args&& ... args);

		template<class T>
		void _log(const T& obj);
		
		void _log();
	public:
		Log(std::string filename, bool console = false);
		explicit Log(bool console = false);
		Log(Log&& log) noexcept;
		Log& operator =(Log&& log) = default;
		
		~Log();

		void open(std::string filename);
		void close();
		
		template<class ... Args>
		void write(Args&& ... args);

		template<class ... Args>
		void write_important
		(Args&& ... args);
	};


	template <class T, class ... Args>
	void Log::_log(const T& obj, Args&&... args) {
		_log(obj);
		_log(std::forward<Args>(args)...);
	}

	template <class T>
	void Log::_log(const T& obj) {
		m_output << obj;
		if (m_console)
			std::cout << obj;
	}

	inline BinaryOutput::BinaryOutput(const void* pointer, const size_t size):
		m_pointer(pointer), m_size(size){
	}

	inline void Log::_log() {
		m_output << std::endl;
		if (m_console)
			std::cout << std::endl;
	}

	inline Log::Log(const std::string filename, bool console):
		m_output(filename, std::ios_base::app), m_console(console) {
	}

	inline Log::Log(bool console):
		m_console(console) {
	}

	inline Log::Log(Log&& log) noexcept:
		m_output(std::move(log.m_output)), m_console(log.m_console){
	}

	inline Log::~Log() {
		m_output.close();
	}

	inline void Log::open(std::string filename) {
		m_output.open(filename, std::ios_base::app);
	}

	inline void Log::close() {
		m_output.close();
	}

	template <class ... Args>
	void Log::write(Args&&... args) {
		try {
			using namespace std::chrono;
			
			auto time = system_clock::to_time_t(system_clock::now());
			auto pointer = std::make_unique<char[]>(64);
			ctime_s(pointer.get(), 64, &time);
			pointer[strlen(pointer.get()) - 1] = '\0';
			
			_log("[", pointer.get(), "]\t");
			_log(std::forward<Args>(args)...);
			_log();
		}
		catch (std::exception& ex) {
			std::cout << "Ошибка при записи в лог\n";
			std::cout << ex.what() << std::endl;
		}
	}

	template <class ... Args>
	void Log::write_important(Args&&... args) {
		try {
			using namespace std::chrono;
			_log('\n');
			
			auto time = system_clock::to_time_t(system_clock::now());
			auto pointer = std::make_unique<char[]>(64);
			ctime_s(pointer.get(), 64, &time);
			pointer[strlen(pointer.get()) - 1] = '\0';
			
			_log("IMPORTANT! [", pointer.get(), "]\t");
			_log(std::forward<Args>(args)...);
			_log('\n');
			_log();
		}
		catch (std::exception& ex) {
			std::cout << "Ошибка при записи в лог\n";
			std::cout << ex.what() << std::endl;
		}
	}
}

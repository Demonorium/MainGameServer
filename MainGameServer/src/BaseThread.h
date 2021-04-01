#pragma once
#include <iostream>
#include<mutex>
#include<thread>

namespace demonorium
{
	/**
	 * \brief ���������� ��� std::thread
	 */
	class BaseThread {
		static void runThread(BaseThread* thread);
		std::atomic<bool> m_deconstruction;
		std::atomic<bool> m_condition;
		std::atomic<bool> m_awaiting;
	protected:
		std::thread* m_thread;
		std::mutex m_thread_mutex;
		std::condition_variable m_cv;
		
		virtual void onInit() = 0;	//���������� �� ����� ������
		virtual void onFrame() = 0;	//���������� � ����� ������
		virtual void onPause();		//���������� ����� pause(), ���� �� ������� destroyThread()
		virtual void onUnPause();	//���������� ����� run(), ���� �� ������� destroyThread()
		virtual void onDestruction(); //���������� ����� ���������� ������
		
		bool isRealyPaused() const;
	public:
		BaseThread();
		virtual ~BaseThread();

		void loop();	//���� ������
		void start();	//������ ������
		void destroyThread(); //������ ��������� � ����������� ������� ������, ���� �� ������
		
		void pause(); //������������� ���������� ������, ����� ��� ���������� �������
		void run(); //���������� ���������� ������, ��� �� ����� notify ���� �� �������

		bool isRunning() const;	//����� ������ �����������?
		bool containsThread() const; //�������� � ���� ��������� �����?
		
	};


	inline void BaseThread::runThread(BaseThread* thread) {
		thread->loop();
	}

	inline bool BaseThread::isRealyPaused() const {
		return m_awaiting;
	}

	inline BaseThread::BaseThread():
		m_condition(false), m_deconstruction(false),
		m_thread(nullptr), m_awaiting(false) {
	}

	inline BaseThread::~BaseThread() {
		if (m_thread) {
			destroyThread();
		}
	}

	inline void BaseThread::loop() {
		onInit();
		m_awaiting = false;
		while (!m_deconstruction) {
			while (m_condition)
				onFrame();

			if (m_deconstruction)
				break;

			
			std::unique_lock<std::mutex> lk(m_thread_mutex);
			m_awaiting = true;
			onPause();
			m_cv.wait(lk);

			if (m_deconstruction)
				break;
			
			onUnPause();
			m_awaiting = false;
		}
		onDestruction();
	}

	inline void BaseThread::start() {
		if (m_thread) {
			run(); 
		} else {
			m_condition.store(true);
			m_deconstruction.store(false);
			m_thread = new std::thread(runThread, this);
		}
	}

	inline void BaseThread::pause() {
		m_condition.store(false);
	}

	inline void BaseThread::onPause() {
	}

	inline void BaseThread::onUnPause() {
	}

	inline void BaseThread::onDestruction() {
	}

	inline void BaseThread::run() {
		m_condition.store(true);
		while (m_awaiting)
			m_cv.notify_one();
	}

	inline bool BaseThread::isRunning() const {
		return !m_awaiting && m_condition;
	}

	inline bool BaseThread::containsThread() const {
		return (m_thread != nullptr) && !m_deconstruction;
	}

	inline void BaseThread::destroyThread() {
		m_deconstruction.store(true);
		
		if (m_awaiting)
			m_cv.notify_one();
		else if (m_condition)
			pause();

		if (m_thread) {
			if (m_thread->joinable())
				m_thread->join();

			m_deconstruction.store(false);
			
			delete m_thread;
			m_thread = nullptr;
		} else {
			m_deconstruction.store(false);
		}
	}
}

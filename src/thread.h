#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

namespace Seraphina
{
	struct ThreadData;

	class ThreadPool
	{
	private:
		ThreadData* threads[1024];
		int threadnum = 1;

		std::mutex mutex;
		std::condition_variable cv;
		std::atomic_uchar ponder, stop;

	public:
		void init();
		void SetThreadNum(int n);
		void CreateThread();
		void DestroyThread();
		void Wait(ThreadData& thread);
		void WakeUp();
		void IdleLoop();

		void MainThread();
		void OtherThreads();
	};
}
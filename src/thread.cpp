#include "thread.h"
#include "bitboard.h"

namespace Seraphina
{
	struct ThreadData
	{
		int id, multipv, depth, seldepth, nodes, tbhits;
		std::mutex mutex;
		std::condition_variable cv;

		Board board;
	};

	ThreadPool threadpool;

	void ThreadPool::init()
	{
		ponder = stop = 0;
	}

	void ThreadPool::SetThreadNum(int n)
	{
		n == 0 ? threadnum = 1 : threadnum = n;
	}

	void ThreadPool::CreateThread()
	{

	}

	void ThreadPool::DestroyThread()
	{
		for (int i = 0; i < threadnum; i++)
		{
			delete threads[i];
		}
	}

	void ThreadPool::Wait(ThreadData& thread)
	{
		std::unique_lock<std::mutex> lock(thread.mutex);
		cv.wait(lock);
	}

	void ThreadPool::WakeUp()
	{
		cv.notify_all();
	}

	void ThreadPool::IdleLoop()
	{
		
	}

	void ThreadPool::MainThread()
	{
		// Main Thread
	}

	void ThreadPool::OtherThreads()
	{
		// Other Threads
	}
}
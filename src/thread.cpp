// Seraphina is an Open-Source NNUE (Efficiently Updatable Neural Network) UCI Chess Engine
// Features: Magic Bitboard, Alpha-Beta Pruning, NNUE, etc
// Requriements: 64-bits Computers, Multiple CPU Architecture Support, Microsoft Windows or Linux Operating System
// Seraphina's NNUE is trained by Grapheus, syzygy tablebase usage code from Fathom
// Programmed By Henry Z
// Special thanks to Luecx, Zomby, Slender(rafid-dev) and other Openbench Discord Members for their generous help of Seraphina NNUE training

#include "thread.h"
#include "bitboard.h"

namespace Seraphina
{
	uint64_t ThreadPool::get_nodes()
	{
		uint64_t n = 0;

		for (auto& t : threads)
		{
			n += t->nodes;
		}

		return n;
	}

	uint64_t ThreadPool::get_tbhits()
	{
		uint64_t n = 0;

		for (auto& t : threads)
		{
			n += t->tbhits;
		}

		return n;
	}

	void ThreadPool::set_threads(int n)
	{
		n <= 0 ? num = 1 : num = n;

		threads.reserve(num);
		Thread* thread;

		for (int i = 0; i < num; ++i)
		{
			threads.emplace_back(thread);
		}
	}

	void ThreadPool::kill_threads()
	{
		for (int i = 0; i < num; i++)
		{
			delete threads[i];
		}
	}

	void ThreadPool::wait(Thread& thread)
	{
		std::unique_lock lock(thread.mutex);
		thread.cv.wait(lock);
	}

	void ThreadPool::wait()
	{
		for (auto& t : threads)
		{
			std::unique_lock lock(t->mutex);
			t->cv.wait(lock);
			t->searching = false;
		}
	}

	void ThreadPool::wakeup()
	{
		for (auto& t : threads)
		{
			t->mutex.lock();
			t->searching = true;
			t->mutex.unlock();
			t->cv.notify_all();
		}
	}

	Thread* ThreadPool::main_thread()
	{
		return threads[0];
	}
}
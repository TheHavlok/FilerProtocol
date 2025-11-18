#pragma once

#include "net_common.hpp"

template<typename T>
class queue
{
public:
	queue() = default;
	queue(const queue<T>&) = delete;
	virtual ~queue() { clear(); }

	// Возвращает и сохраняет элемент в начале очереди
	const T& front()
	{
		std::scoped_lock lock(muxQueue);
		return deqQueue.front();
	}

	// Возвращает и сохраняет элемент в конце очереди
	const T& back()
	{
		std::scoped_lock lock(muxQueue);
		return deqQueue.back();
	}

	// Удаляет и возвращает элемент из начала очереди
	T pop_front()
	{
		std::scoped_lock lock(muxQueue);
		auto t = std::move(deqQueue.front());
		deqQueue.pop_front();
		return t;
	}

	// Удаляет и возвращает элемент из конца очереди
	T pop_back()
	{
		std::scoped_lock lock(muxQueue);
		auto t = std::move(deqQueue.back());
		deqQueue.pop_back();
		return t;
	}

	// Добавляет элемент в начало очереди
	void push_front(const T& item)
	{
		std::scoped_lock lock(muxQueue);
		deqQueue.emplace_front(std::move(item));

		std::unique_lock<std::mutex> ul(muxBlocking);
		cvBlocking.notify_one();
	}

	// Добавляет элемент в конец очереди
	void push_back(const T& item)
	{
		std::scoped_lock lock(muxQueue);
		deqQueue.emplace_back(std::move(item));

		std::unique_lock<std::mutex> ul(muxBlocking);
		cvBlocking.notify_one();
	}

	// Возвращает значение true, если в очереди нет элементов
	bool empty()
	{
		std::scoped_lock lock(muxQueue);
		return deqQueue.empty();
	}

	// Возвращает количество элементов в очереди
	size_t count()
	{
		std::scoped_lock lock(muxQueue);
		return deqQueue.size();
	}

	void clear()
	{
		std::scoped_lock lock(muxQueue);
		deqQueue.clear();
	}

	void wait()
	{
		while (empty())
		{
			std::unique_lock<std::mutex> ul(muxBlocking);
			cvBlocking.wait(ul);
		}
	}

protected:
	std::mutex muxQueue;
	std::deque<T> deqQueue;
	std::condition_variable cvBlocking;
	std::mutex muxBlocking;
};

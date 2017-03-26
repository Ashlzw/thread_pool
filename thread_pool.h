#ifndef THREAD_POOL
#define THREAD_POOL

#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <future>
#include <memory>
#include <stdexcept>
#include <condition_variable>
#include <functional>
#include <utility>

class thread_pool
{
public:
	thread_pool(size_t threads);

	template<typename F, typename... Args>
	auto enqueue(F&& f, Args&&... args)
		->std::future<typename std::result_of<F(Args...)>::type>;

	~thread_pool();

private:
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;

	std::mutex queue_mutex; // 互斥锁
	std::condition_variable condition; // 互斥条件变量

	bool stop;
};

#endif // !THREAD_POOL

template<typename F, typename ...Args>
auto thread_pool::enqueue(F && f, Args && ...args) -> std::future<typename std::result_of<F(Args ...)>::type>
{
	using return_type = typename std::result_of < F(Args...) > ::type;

	auto task = std::make_shared<std::packaged_task<return_type()>>(
		std::bind(std::forward<F>(f), std::forward<Args>(args)...)
	);

	std::future<return_type> res = task->get_future();

	{
		if (stop)
			throw std::runtime_error("enqueue on stopped thread_pool");

		tasks.emplace([task] {(*task)();});
	}

	condition.notify_one();

	return res;
}

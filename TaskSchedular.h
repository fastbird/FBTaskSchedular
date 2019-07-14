#pragma once
#include <condition_variable>
#include <list>
#include <vector>

namespace fb
{
	class FTaskSchedular;

	enum class ETaskPriority
	{
		VeryHigh, 
		High, 
		Normal,
		Low,
		VeryLow,
		Count
	};

	class FTask;
	class ITaskListener {
	public:
		virtual void OnTaskFinish(FTask* task) {}
	};

	class FTask : public ITaskListener
	{
	public:
		std::vector<ITaskListener*> TaskListeners;
		bool AutoDelete = false;

		virtual void Run() = 0;
		virtual bool CanRun() const = 0;
		void NotifyFinish()
		{
			for (auto listener : TaskListeners) {
				listener->OnTaskFinish(this);
			}
		}
	};

	class FTaskThread
	{
	public:
		FTaskSchedular& TaskSchedular;
		std::thread Thread;
		volatile bool Quit = false;
		FTask* Task = nullptr;
		std::condition_variable CV;
		std::mutex Mutex;
	
		FTaskThread(FTaskSchedular& taskSchedular);
		void EnterLoop();
	};

	class FTaskSchedular
	{
		std::mutex Mutex;
		std::list<FTask*> Tasks[(int)ETaskPriority::Count];
		std::vector<FTaskThread*> TaskThreads;
		bool Finish = true;
		bool Quit = false;
		std::condition_variable CVFinish;

	public:
		void CreateTaskThreads(int numTaskThreads);
		void FinishTaskThreads();
		void AddTask(FTask* task, ETaskPriority priority);
		void Wait();

	private:
		friend void TaskThread(FTaskThread* taskThread);
		FTask* PopTask();


	};
}
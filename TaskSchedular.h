#pragma once
#include <condition_variable>
#include <queue>
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

	class FTask
	{
	public:
		bool AutoDelete = false;
		virtual void Run() = 0;
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
		std::queue<FTask*> Tasks[(int)ETaskPriority::Count];
		std::vector<FTaskThread*> TaskThreads;

	public:
		void CreateTaskThreads(int numTaskThreads);
		void FinishTaskThreads();
		void AddTask(FTask* task, ETaskPriority priority);

	private:
		friend void TaskThread(FTaskThread* taskThread);
		FTask* PopTask();


	};
}
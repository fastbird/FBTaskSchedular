#pragma once
#include "TaskSchedular.h"
#include <chrono>
#include <assert.h>
namespace fb
{

FTaskThread::FTaskThread(FTaskSchedular& taskSchedular)
	: TaskSchedular(taskSchedular)
{

}

void TaskThread(FTaskThread* taskThread) {
	using namespace std::chrono_literals;
	while (!taskThread->Quit) {
		while (taskThread->Task) {
			taskThread->Task->Run();
			if (taskThread->Task->AutoDelete) {
				delete taskThread->Task;
			}
			taskThread->Task = taskThread->TaskSchedular.PopTask();
		}
		{
			std::unique_lock lock(taskThread->Mutex);
			while (!taskThread->Quit && !taskThread->Task) {
				taskThread->CV.wait_for(lock, 1s, [&taskThread] {return taskThread->Task != nullptr; });
			}
		}
	}
}

void FTaskThread::EnterLoop()
{
	Thread = std::thread(TaskThread, this);
}

/* ----------------------- */
/* Start of FTaskSchedular */
/* ----------------------- */
void FTaskSchedular::CreateTaskThreads(int numTaskThreads)
{
	for (int i = 0; i < numTaskThreads; ++i) {
		TaskThreads.push_back(new FTaskThread(*this));
		TaskThreads.back()->EnterLoop();
	}
}

void FTaskSchedular::FinishTaskThreads() {
	Quit = true;
	for (auto taskThread : TaskThreads) {
		std::unique_lock lock(taskThread->Mutex);
		taskThread->Quit = true;
	}

	for (auto taskThread : TaskThreads) {
		try {
			taskThread->Thread.join();
		}
		catch (...) {
		}
	}
}

void FTaskSchedular::AddTask(FTask* task, ETaskPriority priority)
{
	std::unique_lock lock(Mutex);
	Finish = false;
	if (task->CanRun()) {
		for (auto taskThread : TaskThreads) {
			std::unique_lock lock(taskThread->Mutex);
			if (!taskThread->Task && !taskThread->Quit) {
				taskThread->Task = task;
				lock.unlock();
				taskThread->CV.notify_one();
				return;
			}
		}
	}
	assert(priority != ETaskPriority::Count);
	Tasks[(int)priority].push_back(task);
}

void FTaskSchedular::Wait()
{
	using namespace std::chrono_literals;
	while (!Quit) {
		std::unique_lock lock(Mutex);
		CVFinish.wait_for(lock, 1s, [this]() {return Finish; });
	}
}

FTask* FTaskSchedular::PopTask()
{
	std::unique_lock lock(Mutex);
	bool foundPending = false;
	for (int i = 0; i < (int)ETaskPriority::Count; ++i) {
		for (auto it = Tasks[i].begin(); it != Tasks[i].end(); ++it) {
			if ((*it)->CanRun()) {
				auto ret = *it;
				Tasks[i].erase(it);
				return ret;
			}
			else {
				foundPending = true;
			}
		}
	}
	Finish = true;
	if (foundPending)
	{
		assert(0 && "Possibly, it is a dependency error");
	}
	lock.unlock();
	CVFinish.notify_one();

	return nullptr;
}

} // namespace fb
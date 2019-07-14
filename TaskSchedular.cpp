#pragma once
#include "TaskSchedular.h"
#include <chrono>
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
	for (int i = 0; i <= (int)priority; ++i) {
		if (!Tasks[i].empty()) {
			Tasks[(int)priority].push(task);
			return;
		}
	}

	for (auto taskThread : TaskThreads) {
		std::unique_lock lock(taskThread->Mutex);
		if (!taskThread->Task && !taskThread->Quit) {
			taskThread->Task = task;
			lock.unlock();
			taskThread->CV.notify_one();
			return;
		}
	}

	Tasks[(int)priority].push(task);
}

FTask* FTaskSchedular::PopTask()
{
	std::unique_lock lock(Mutex);
	for (int i = 0; i < (int)ETaskPriority::Count; ++i) {
		if (!Tasks[i].empty()) {
			auto front = Tasks[i].front();
			Tasks[i].pop();
			return front;
		}
	}
	return nullptr;
}

} // namespace fb
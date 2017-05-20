#include "StableHeaders.h"
#include "GO_TaskSchedulerNew.h"

#include "GO_ThreadPool.h"
#include "GO_Profiler.h"
#include "GO_ProfilerTags.h"

namespace GO
{
	TaskSchedulerNew::TaskSchedulerNew(GO::ThreadPool& aThreadPool, unsigned int aMaximumIdentifier, GO_APIProfiler* aProfiler)
		: myThreadPool(aThreadPool)
		, myAreAllTasksComplete(false)
		, myProfiler(aProfiler)
	{
		myTasks.resize(aMaximumIdentifier);
		myTaskDependencies.resize(aMaximumIdentifier);
		myTasksBlockedByCounts.resize(aMaximumIdentifier);
		myTaskCompletedFlags.resize(aMaximumIdentifier);
	}


	void TaskSchedulerNew::addTask(Task aTask)
	{
		GO::AssertMutexLock assertLock(myTaskListAssertMutex);
		myTasks[aTask.getUniqueID()] = aTask;
	}

	void TaskSchedulerNew::addTask(Task aTask, unsigned int aTaskDependencyUniqueID)
	{
		GO::AssertMutexLock assertLock(myTaskListAssertMutex);
		myTasks[aTask.getUniqueID()] = aTask;

		{
			std::lock_guard<std::mutex> lock(myTaskCompleteMutex);
			myTaskDependencies[aTaskDependencyUniqueID].addDependentTask(aTask.getUniqueID());
			myTasksBlockedByCounts[aTask.getUniqueID()]++;
		}
	}

	void TaskSchedulerNew::runPendingTasks()
	{
		std::unique_lock<std::mutex> lock(myTaskCompleteMutex);
		submitUnblockedTasks();

		// TODO(scarroll): This should be a store/reset operation so the previous tag goes back on the profiler
		myProfiler->PushThreadEvent(GO_ProfilerTags::THREAD_TAG_WAITING);
		myAllTasksCompletedCondition.wait(lock,
			[this]
		{
			return myAreAllTasksComplete;
		});
	}

	int TaskSchedulerNew::runTaskInternal(Task* aTask)
	{
		myProfiler->PushThreadEvent(aTask->getTaskTag());
		(*aTask)();

		notifyTaskComplete(*aTask);
		return 1;
	}

	void TaskSchedulerNew::notifyTaskComplete(Task& aTask)
	{
		// TODO: This is a bad idea because all of the pooled threads will be calling into this
		//		 and creating a lot of contention
		std::lock_guard<std::mutex> lock(myTaskCompleteMutex);
		const TaskDependencies& taskDependency = myTaskDependencies[aTask.getUniqueID()];

		myTaskCompletedFlags[aTask.getUniqueID()] = true;

		notifyDependencyComplete(taskDependency);

		submitUnblockedTasks();

		signalIfAllTasksComplete();
	}

	void TaskSchedulerNew::signalIfAllTasksComplete()
	{
		GO_ASSERT(myAreAllTasksComplete == false, "Checking for task completion after all tasks supposedly complete");

		GO::AssertMutexLock assertLock(myTaskListAssertMutex);
		for (unsigned int i = 0; i < myTaskCompletedFlags.size(); i++)
		{
			if (!myTaskCompletedFlags[i])
			{
				return;
			}
		}

		myAreAllTasksComplete = true;
		myAllTasksCompletedCondition.notify_all();
	}

	void TaskSchedulerNew::notifyDependencyComplete(const TaskDependencies& aTaskDependency)
	{
		GO::AssertMutexLock assertLock(myTaskListAssertMutex);
		for (unsigned int i = 0; i < aTaskDependency.myNumDependentTasks; i++)
		{
			unsigned int& blockedCount = myTasksBlockedByCounts[aTaskDependency.myDependentTaskUniqueIDs[i]];
			GO_ASSERT(blockedCount > 0, "Trying to unblock a task that was already unblocked");
			blockedCount--;
		}
	}

	void TaskSchedulerNew::submitUnblockedTasks()
	{
		GO::AssertMutexLock assertLock(myTaskListAssertMutex);

		for (unsigned int i = 0; i < myTasks.size(); i++)
		{
			Task& currentTask = myTasks[i];

			if (!currentTask.hasStarted() && !currentTask.isComplete() && myTasksBlockedByCounts[currentTask.getUniqueID()] == 0)
			{
				currentTask.markAsStarted();
				myThreadPool.Submit(std::bind(&TaskSchedulerNew::runTaskInternal, this, &currentTask));
			}
		}
	}
}
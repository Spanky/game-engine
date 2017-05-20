#pragma once

#include "GO_AssertMutex.h"

class GO_APIProfiler;

namespace GO
{
	class ThreadPool;

	// TODO(scarroll): Create a method to package up all of this into a job with an identifiable tag
	//					Other jobs should be able to wait on this job for completion so we do not have to
	//					handle the scheduling here.
	//					Dependencies from this job to other jobs will need to be created as well. Ideally
	//					the tags should be created outside of the submission process so that the order
	//					of submission does not necessarily matter.
	class Task
	{
	private:
		static constexpr unsigned int InvalidTaskUniqueID = 0xffffffff;
		static constexpr unsigned char InvalidThreadTag = 0xff;

	private:
		unsigned int myUniqueID;
		std::function<void()> myTask;
		unsigned char myTaskTag;
		bool myIsComplete;
		bool myHasStarted;

	public:
		Task()
			: myUniqueID(InvalidTaskUniqueID)
			, myTaskTag(InvalidThreadTag)
			, myIsComplete(true)
			, myHasStarted(true)
		{

		}
		Task(unsigned int aUniqueID, std::function<void()> aFunction, unsigned char aTaskTag)
			: myUniqueID(aUniqueID)
			, myTask(std::move(aFunction))
			, myTaskTag(aTaskTag)
			, myIsComplete(false)
			, myHasStarted(false)
		{
		}

		unsigned int getUniqueID() const
		{
			return myUniqueID;
		}

		unsigned char getTaskTag() const
		{
			return myTaskTag;
		}

		bool isComplete() const
		{
			return myIsComplete;
		}

		bool hasStarted() const
		{
			return myHasStarted;
		}

		void markAsStarted()
		{
			myHasStarted = true;
		}

		void operator()()
		{
			myTask();

			// TODO(scarroll): How do I make sure that this happens after myTask finishes?
			//					Out of order execution may re-order these
			myIsComplete = true;
		}
	};



	struct TaskDependencies
	{
	public:
		static constexpr unsigned short MaxDependencies = 4;

		// The unique IDs of the tasks that are dependent on the associated task.
		// Once the associated task is complete, these tasks may move to an unblocked
		// state if they have no other dependencies
		unsigned int myDependentTaskUniqueIDs[MaxDependencies];
		unsigned char myNumDependentTasks;

	public:
		TaskDependencies()
			: myNumDependentTasks(0)
		{
			const int memorySize = sizeof(myDependentTaskUniqueIDs);
			memset(myDependentTaskUniqueIDs, 0, sizeof(myDependentTaskUniqueIDs));
		}

		void addDependentTask(unsigned int aDependentTaskUniqueID)
		{
			GO_ASSERT(myNumDependentTasks < MaxDependencies, "Too many task dependencies");
			myDependentTaskUniqueIDs[myNumDependentTasks++] = aDependentTaskUniqueID;
		}
	};




	class TaskSchedulerNew
	{
	private:
		std::vector<Task> myTasks;
		ThreadPool& myThreadPool;

		std::vector<TaskDependencies> myTaskDependencies;
		std::vector<unsigned int> myTasksBlockedByCounts;
		std::vector<bool> myTaskCompletedFlags;

		std::mutex myTaskCompleteMutex;
		std::condition_variable myAllTasksCompletedCondition;
		bool myAreAllTasksComplete;

		AssertMutex myTaskListAssertMutex;
		GO_APIProfiler* myProfiler;

	public:
		TaskSchedulerNew(ThreadPool& aThreadPool, unsigned int aMaximumIdentifier, GO_APIProfiler* aProfiler);

		void addTask(Task aTask);
		void addTask(Task aTask, unsigned int aTaskDependencyUniqueID);
		void addTask(Task aTask, unsigned int aTaskDependencyUniqueID, unsigned int aSecondTaskDependencyUniqueID);
		void runPendingTasks();

	private:
		int runTaskInternal(Task* aTask);
		void submitUnblockedTasks();

		void notifyTaskComplete(Task& aTask);
		void notifyDependencyComplete(const TaskDependencies& aTaskDependency);

		void signalIfAllTasksComplete();
	};
}

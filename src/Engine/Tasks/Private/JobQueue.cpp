#include "Tasks/JobQueue.h"

#include <algorithm>
#include <utility>

namespace engine::tasks::detail
{
    void JobQueue::Push(
        QueuedTask task)
    {
        const std::size_t index =
            ToIndex(task.priority);

        if (index >= queues_.size())
        {
            task.priority =
                TaskPriority::Normal;
        }

        queues_[
            ToIndex(task.priority)]
                .push_back(
                    std::move(task));

        ++taskCount_;
    }

    bool JobQueue::TryPop(
        QueuedTask& task)
    {
        for (std::size_t index = queues_.size();
             index > 0;
             --index)
        {
            std::deque<QueuedTask>& queue =
                queues_[index - 1];

            if (queue.empty())
            {
                continue;
            }

            task =
                std::move(
                    queue.front());

            queue.pop_front();

            --taskCount_;

            return true;
        }

        return false;
    }

    bool JobQueue::RemoveById(
        const std::uint64_t taskId,
        QueuedTask& removedTask)
    {
        for (std::deque<QueuedTask>& queue :
             queues_)
        {
            const auto iterator =
                std::find_if(
                    queue.begin(),
                    queue.end(),
                    [taskId](
                        const QueuedTask& task)
                    {
                        return task.id == taskId;
                    });

            if (iterator == queue.end())
            {
                continue;
            }

            removedTask =
                std::move(*iterator);

            queue.erase(iterator);

            --taskCount_;

            return true;
        }

        return false;
    }

    std::vector<QueuedTask>
        JobQueue::TakeAll()
    {
        std::vector<QueuedTask> tasks;

        tasks.reserve(taskCount_);

        for (std::deque<QueuedTask>& queue :
             queues_)
        {
            while (!queue.empty())
            {
                tasks.push_back(
                    std::move(
                        queue.front()));

                queue.pop_front();
            }
        }

        taskCount_ = 0;

        return tasks;
    }

    std::size_t
        JobQueue::GetSize() const noexcept
    {
        return taskCount_;
    }

    bool JobQueue::IsEmpty() const noexcept
    {
        return taskCount_ == 0;
    }
}
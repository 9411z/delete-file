class Timer {
private:
    TimerTaskQueue queue;
    bool newTasksMayBeScheduled;
    std::mutex lock;
    std::thread worker_;
    std::condition_variable condition;

public:
    ~Timer() {
        finalize();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    Timer() : newTasksMayBeScheduled(true) {}

    template <typename T = TimerTaskPtr>
    void schedule(T &&task, int64_t delay, int64_t period = 0) {
        if (delay < 0 || period < 0)
            return;

        sched(std::forward<T>(task), delay, period);
    }

    template <typename T = TimerTaskPtr>
    void scheduleAtFixedRate(T &&task, int64_t delay, int64_t period) {
        if (delay < 0)
            return;
        if (period <= 0)
            return;
        sched(std::forward<T>(task), delay, period);
    }
    void cancel() {
        std::lock_guard<std::mutex> guard(lock);
        newTasksMayBeScheduled = false;
        notify(); // In case queue was already empty.
    }
    void start() {
        worker_ = std::thread([this]() {
            mainLoop();
        });
    }

    void removeCancelTask() {
        while (!queue.empty()) {
            if (queue.top()->State() == CANCELLED) {
                queue.pop(); // 删除
            } else {
                break;
            }
        }
    }

private:
    void mainLoop() {
        while (true) {
            TimerTaskPtr task;
            bool taskFired = false;
            {
                std::unique_lock<std::mutex> lk(lock);
                // Wait for queue to become non-empty
                while (queue.empty() && newTasksMayBeScheduled) {
                    condition.wait(lk, [this] {
                        return !this->queue.empty() || !newTasksMayBeScheduled;
                    });
                }

                if (!newTasksMayBeScheduled)
                    break;

                removeCancelTask();
                if (queue.empty())
                    continue;

                // Queue nonempty; look at first evt and do the right thing
                MillisDuration_t waitFor(0);
                task.reset();
                task = queue.top();
                {
                    TimePoint_t currentTime = Clock_t::now();
                    waitFor                 = std::chrono::duration_cast<MillisDuration_t>(
                        task->schedTime() - currentTime);
                    if (waitFor.count() <= 0) {
                        queue.pop(); // 删除
                        taskFired = true;
                        if (task->repeatPeriod().count() <= 0) { // Non-repeating, remove
                            task->setState(EXECUTED);
                        } else { // Repeating task, reschedule
                            task->reload();
                            queue.push(task); // Re queue task!
                        }
                    }
                }

                if (!taskFired && waitFor.count() > 0) { // Task hasn't yet fired; wait
                    condition.wait_for(lk, waitFor);
                }
            }
            if (taskFired) // Task fired; run it, holding no locks
                task->run(task);
        }
        {
            std::lock_guard<std::mutex> guard(this->lock);
            this->newTasksMayBeScheduled = false;
        }
        MLOG(Bag, INFO) << "end timer mainloop";
    }
    void finalize() {
        std::lock_guard<std::mutex> guard(lock);
        newTasksMayBeScheduled = false;
        notify();
    }

    inline void notify() { condition.notify_one(); }

    template <typename T = TimerTaskPtr> void sched(T &&task, int64_t delay, int64_t period) {
        std::lock_guard<std::mutex> guard(lock);
        if (!newTasksMayBeScheduled)
            return;

        task->setSchedTime(Clock_t::now() + MillisDuration_t(delay));
        task->setRepeatPeriod(MillisDuration_t(period));
        task->setState(SCHEDULED);

        queue.push(std::forward<T>(task));
        notify();
    }
};

//
// Created by ackav on 16-04-2020.
//

#ifndef ANDROID_V8HACK_H
#define ANDROID_V8HACK_H

#include "common.h"
#include "V8Context.h"

class V8HackedTaskRunner;

class V8Task {
public:
    std::unique_ptr<Task> task;
    V8HackedTaskRunner* taskRunner;
};

class V8HackedTaskRunner: public v8::TaskRunner {

public:

    V8HackedTaskRunner(
            std::shared_ptr<TaskRunner> pTaskRunner,
            QueueTask pQueueTask) {
        taskRunner = pTaskRunner;
        queueTask = pQueueTask;
    }

    virtual void PostTask(std::unique_ptr<Task> task) override {
        taskRunner->PostTask(std::move(task));
    }

    virtual void PostDelayedTask(std::unique_ptr<Task> task,
                                 double delay_in_seconds) override {
        if (delay_in_seconds == 0) {
            taskRunner->PostDelayedTask(std::move(task), 0);
        } else {
            V8Task* t = new V8Task();
            t->taskRunner = this;
            t->task = std::move(task);
            queueTask((void*)t, delay_in_seconds);
        }
    }

    virtual void PostIdleTask(std::unique_ptr<IdleTask> task) override {}

    virtual bool IdleTasksEnabled() override {
        return false;
    }
private:
    std::shared_ptr<TaskRunner> taskRunner;
    QueueTask queueTask;
};

class TV8Platform : public v8::Platform
{
    using Isolate = v8::Isolate;
    using Task = v8::Task;
    using IdleTask = v8::IdleTask;
    using PageAllocator = v8::PageAllocator;
    using TaskRunner = v8::TaskRunner;
    using TracingController = v8::TracingController;

public:
    TV8Platform(QueueTask queueTask)
    {
        mQueueTask = queueTask;
        mPlatform = v8::platform::NewDefaultPlatform();

    }

    virtual PageAllocator* GetPageAllocator() override {	return mPlatform->GetPageAllocator();	}
    virtual void OnCriticalMemoryPressure() override {	mPlatform->OnCriticalMemoryPressure();	}
    virtual bool OnCriticalMemoryPressure(size_t length) override {	return mPlatform->OnCriticalMemoryPressure(length);	}
    virtual int NumberOfWorkerThreads() override{	return mPlatform->NumberOfWorkerThreads();	}
    virtual std::shared_ptr<TaskRunner> GetForegroundTaskRunner(Isolate* isolate) override{
        V8Context* context = (V8Context*)isolate->GetData(0);
        if(context->foregroundTaskRunner.get() == nullptr) {
            V8HackedTaskRunner* tr = new V8HackedTaskRunner(mPlatform->GetForegroundTaskRunner(isolate), mQueueTask);
            std::shared_ptr<TaskRunner> ptr(tr);
            context->foregroundTaskRunner.swap(ptr);
        }
        return context->foregroundTaskRunner;
    }
    virtual std::shared_ptr<TaskRunner> GetBackgroundTaskRunner(Isolate* isolate) override{
        V8Context* context = (V8Context*)isolate->GetData(0);
        if(context->backgroundTaskRunner.get() == nullptr) {
            V8HackedTaskRunner* tr = new V8HackedTaskRunner(mPlatform->GetBackgroundTaskRunner(isolate), mQueueTask);
            std::shared_ptr<TaskRunner> ptr(tr);
            context->backgroundTaskRunner.swap(ptr);
        }
        return context->backgroundTaskRunner;
    }
    virtual std::shared_ptr<TaskRunner> GetWorkerThreadsTaskRunner(Isolate* isolate) override {
        V8Context* context = (V8Context*)isolate->GetData(0);
        if(context->workerTaskRunner.get() == nullptr) {
            V8HackedTaskRunner* tr = new V8HackedTaskRunner(mPlatform->GetWorkerThreadsTaskRunner(isolate), mQueueTask);
            std::shared_ptr<TaskRunner> ptr(tr);
            context->workerTaskRunner.swap(ptr);
        }
        return context->workerTaskRunner;
    }
    virtual size_t NumberOfAvailableBackgroundThreads() override {
        return mPlatform->NumberOfAvailableBackgroundThreads();
    }
    virtual void CallOnWorkerThread(std::unique_ptr<Task> task) override{	mPlatform->CallOnWorkerThread( std::move(task) );	}
    virtual void CallBlockingTaskOnWorkerThread(std::unique_ptr<Task> task) override	{	mPlatform->CallBlockingTaskOnWorkerThread( std::move(task) );	}
//    virtual void CallOnBackgroundThread(Task* task) {
//        mPlatform->CallOnBackgroundThread(task);
//    }
    virtual void CallOnBackgroundThread(Task* task, ExpectedRuntime r) override {
        mPlatform->CallOnBackgroundThread(task, r);
    }
    // virtual void CallDelayedOnWorkerThread(std::unique_ptr<Task> task,double delay_in_seconds) override	{	mPlatform->CallDelayedOnWorkerThread( std::move(task),delay_in_seconds);	}
    virtual void CallOnForegroundThread(Isolate* isolate, Task* task) override	{	mPlatform->CallOnForegroundThread(isolate, task);	}
    virtual void CallDelayedOnForegroundThread(Isolate* isolate, Task* task, double delay_in_seconds) override	{
        delay_in_seconds = 0;
        mPlatform->CallDelayedOnForegroundThread( isolate, task, delay_in_seconds );
    }
    virtual void CallIdleOnForegroundThread(Isolate* isolate, IdleTask* task) override	{	mPlatform->CallIdleOnForegroundThread(isolate, task);	}
    virtual bool IdleTasksEnabled(Isolate* isolate) override	{	return mPlatform->IdleTasksEnabled(isolate);	}
    virtual double MonotonicallyIncreasingTime() override	{	return mPlatform->MonotonicallyIncreasingTime();	}
    virtual double CurrentClockTimeMillis() override	{	return mPlatform->CurrentClockTimeMillis();	}
    virtual StackTracePrinter GetStackTracePrinter() override	{	return mPlatform->GetStackTracePrinter();	}
    virtual TracingController* GetTracingController() override	{	return mPlatform->GetTracingController();	}

    //	here's the bodged fix to avoid the assert. Seems to work!
//    virtual void CallDelayedOnWorkerThread(std::unique_ptr<Task> task,double delay_in_seconds) override
//    {
//        delay_in_seconds = 0;
//        mPlatform->CallDelayedOnWorkerThread( std::move(task),delay_in_seconds);
//    }

protected:
    // std::shared_ptr<v8::Platform>	mpPlatform;
    std::unique_ptr<v8::Platform>	mPlatform;
    QueueTask mQueueTask;

};

#endif //ANDROID_V8HACK_H

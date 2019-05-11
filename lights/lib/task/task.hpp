#ifndef TASK_HPP
#define TASK_HPP

#ifndef NORMAL_TASK_PRIORITY
#define NORMAL_TASK_PRIORITY 5
#endif

#ifndef NORMAL_STACK_DEPTH
#define NORMAL_STACK_DEPTH 4096
#endif

#include <tuple>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class Task
{
    TaskHandle_t handle;

  public:
    template <typename F, typename... Args>
    using IsInvocable = std::enable_if_t<std::is_invocable_v<F, Args...>>;

    template <typename F, typename... Args, typename = IsInvocable<F, Args...>>
    explicit Task(const std::string &name,
                  const UBaseType_t &priority,
                  const uint32_t &stackDepth,
                  F &&task, Args &&... args);

    template <typename F, typename... Args, typename = IsInvocable<F, Args...>>
    explicit Task(const std::string &name, F &&task, Args &&... args)
        : Task(name, NORMAL_TASK_PRIORITY, NORMAL_STACK_DEPTH,
               std::forward<F>(task), std::forward<Args>(args)...) {}

    template <typename F, typename... Args, typename = IsInvocable<F, Args...>>
    explicit Task(F &&task, Args &&... args)
        : Task("task", std::forward<F>(task), std::forward<Args>(args)...) {}

    Task(const Task &) = delete;
    Task(Task &&other)
    {
        handle = std::move(other.handle);
        other.handle = nullptr;
    }
    Task &operator=(const Task &) = delete;
    Task &operator=(Task &&other) = delete;

    ~Task()
    {
        if (handle != nullptr)
            vTaskDelete(handle);
    }

    void swap(Task &other)
    {
        std::swap(handle, other.handle);
    }

    template <typename F, typename... Args>
    struct arguments_wrapper
    {
        using Tuple = std::tuple<std::decay_t<Args>...>;
        using Callable = std::decay_t<F>;

        Callable task;
        Tuple args;

        arguments_wrapper(F &&task, Args &&... args)
            : task(std::forward<F>(task)),
              args(std::forward<Args>(args)...)
        {
            static_assert(std::is_invocable_v<F, Args...>);
        }
    };

    template <typename F, typename... Args, typename = IsInvocable<F, Args...>>
    static void taskRoutine(void *arg);
};

template <typename F, typename... Args, typename = Task::IsInvocable<F, Args...>>
Task::Task(const std::string &name,
           const UBaseType_t &priority,
           const uint32_t &stackDepth,
           F &&task, Args &&... args)
{
    static_assert(std::is_invocable_v<F, Args...>);
    using Data = Task::arguments_wrapper<F, Args...>;
    Data *const data = new Data(std::forward<F>(task), std::forward<Args>(args)...);

    if (xTaskCreate(taskRoutine<std::decay_t<F>, std::decay_t<Args>...>,
                    name.c_str(),
                    stackDepth,
                    data,
                    priority,
                    &handle) != pdPASS)
    {
        handle = nullptr;
        delete data;
    }
}

template <typename F, typename... Args, typename = Task::IsInvocable<F, Args...>>
void Task::taskRoutine(void *arg)
{
    {
        using Data = Task::arguments_wrapper<F, Args...>;
        Data *const data = reinterpret_cast<Data *>(arg);
        auto f = std::move(data->task);
        auto args = std::move(data->args);
        delete data;

        std::apply(std::move(f), std::move(args));
    }

    while (true)
        vTaskDelay(portMAX_DELAY);
}

void swap(Task &t1, Task &t2)
{
    t1.swap(t2);
}

#endif

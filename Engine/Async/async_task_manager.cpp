/**
 * @file async_task_manager.cpp
 * @brief Asenkron görev yöneticisi sınıfının iş parçacığı döngüsü ve iş akışı uygulaması.
 */

#include "async_task_manager.h"
#include <iostream>

namespace core {

    async_task_manager& async_task_manager::instance() {
        static async_task_manager s_instance;
        return s_instance;
    }

    async_task_manager::async_task_manager() {
        m_workerThread = std::thread(&async_task_manager::worker_loop, this);
    }

    async_task_manager::~async_task_manager() {
        shutdown();
    }

    void async_task_manager::shutdown() {
        if (!m_stopRequested.exchange(true)) {
            m_cancelCurrent.store(true);
            m_cv.notify_all();

            if (m_workerThread.joinable()) {
                m_workerThread.join();
            }
        }
    }

    void async_task_manager::run_task(
        const std::string& name,
        TaskWorkFn workFn,
        MainThreadCompleteFn onCompleteMainThread)
    {
        if (!workFn) return;

        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_taskQueue.push(TaskItem{ name, std::move(workFn), std::move(onCompleteMainThread) });
            m_isBusy.store(true, std::memory_order_release);
        }

        m_cv.notify_one();
    }

    void async_task_manager::cancel_current_task() {
        m_cancelCurrent.store(true, std::memory_order_relaxed);
    }

    std::string async_task_manager::get_task_name() const {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        return m_currentTaskName;
    }

    std::string async_task_manager::get_status_text() const {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        return m_currentStatusText;
    }

    void async_task_manager::poll_main_thread_callbacks() {
        std::vector<std::function<void()>> callbacksToRun;
        {
            std::lock_guard<std::mutex> lock(m_callbackMutex);
            if (!m_mainThreadCallbacks.empty()) {
                callbacksToRun.swap(m_mainThreadCallbacks);
            }
        }

        for (auto& cb : callbacksToRun) {
            if (cb) {
                try {
                    cb();
                }
                catch (const std::exception& e) {
                    std::cerr << "[AsyncTaskManager] Main thread callback exception: " << e.what() << "\n";
                }
                catch (...) {
                    std::cerr << "[AsyncTaskManager] Main thread callback unknown exception.\n";
                }
            }
        }
    }

    void async_task_manager::worker_loop() {
        while (!m_stopRequested.load(std::memory_order_relaxed)) {
            TaskItem currentItem;
            {
                std::unique_lock<std::mutex> lock(m_taskMutex);
                m_cv.wait(lock, [this]() {
                    return m_stopRequested.load(std::memory_order_relaxed) || !m_taskQueue.empty();
                });

                if (m_stopRequested.load(std::memory_order_relaxed)) {
                    break;
                }

                currentItem = std::move(m_taskQueue.front());
                m_taskQueue.pop();

                m_currentTaskName = currentItem.name;
                m_currentStatusText = currentItem.name;
                m_currentProgress.store(-1.0f, std::memory_order_relaxed);
                m_cancelCurrent.store(false, std::memory_order_relaxed);
            }

            task_reporter reporter(m_currentProgress, m_currentStatusText, m_taskMutex, m_cancelCurrent);
            bool success = false;
            std::string finalMessage = "";

            try {
                success = currentItem.workFn(reporter);
            }
            catch (const std::exception& ex) {
                success = false;
                finalMessage = ex.what();
                std::cerr << "[AsyncTaskManager] Task failed with exception (" << currentItem.name << "): " << ex.what() << "\n";
            }
            catch (...) {
                success = false;
                finalMessage = "Bilinmeyen kritik hata olustu.";
                std::cerr << "[AsyncTaskManager] Task failed with unknown exception (" << currentItem.name << ").\n";
            }

            // Eğer ana iş parçacığı geri çağrımı varsa kuyruğa al
            if (currentItem.onComplete) {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                m_mainThreadCallbacks.push_back([cb = std::move(currentItem.onComplete), success, finalMessage]() {
                    cb(success, finalMessage);
                });
            }

            // Durum güncellemesi
            {
                std::lock_guard<std::mutex> lock(m_taskMutex);
                if (m_taskQueue.empty()) {
                    m_isBusy.store(false, std::memory_order_release);
                    m_currentProgress.store(0.0f, std::memory_order_relaxed);
                    m_currentTaskName.clear();
                    m_currentStatusText.clear();
                }
            }
        }
    }

} // namespace core


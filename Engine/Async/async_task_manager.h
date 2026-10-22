#pragma once

/**
 * @file async_task_manager.h
 * @brief Oyun motoru genelinde CPU/IO ağırlıklı işleri (betik derleme, sahne kaydetme/yükleme,
 * dışa aktarma vb.) ana render döngüsünü dondurmadan arka planda yürüten modern asenkron görev yöneticisi.
 */

#include <string>
#include <functional>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>

namespace core {

    /**
     * @struct task_reporter
     * @brief Çalışan bir asenkron görevin ilerleme durumunu ve durum metnini güncellemesini sağlayan arayüz.
     */
    class task_reporter {
    public:
        task_reporter(
            std::atomic<float>& progress,
            std::string& statusText,
            std::mutex& mutex,
            std::atomic<bool>& cancelRequested)
            : m_progress(progress)
            , m_statusText(statusText)
            , m_mutex(mutex)
            , m_cancelRequested(cancelRequested)
        {
        }

        /**
         * @brief Görevin anlık ilerleme oranını ve açıklama metnini günceller.
         * @param progress 0.0f ile 1.0f arasında değer, veya belirsiz (marquee) için -1.0f.
         * @param status Kullanıcıya gösterilecek anlık durum açıklaması.
         */
        void set_progress(float progress, const std::string& status = "") {
            m_progress.store(progress, std::memory_order_relaxed);
            if (!status.empty()) {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_statusText = status;
            }
        }

        /**
         * @brief Görevin durumunu belirsiz/animasyonlu ilerleme moduna alır.
         * @param status Kullanıcıya gösterilecek anlık durum açıklaması.
         */
        void set_indeterminate(const std::string& status = "") {
            set_progress(-1.0f, status);
        }

        /**
         * @brief Görevin iptal edilip edilmediğini kontrol eder.
         */
        [[nodiscard]] bool is_cancelled() const noexcept {
            return m_cancelRequested.load(std::memory_order_relaxed);
        }

    private:
        std::atomic<float>& m_progress;
        std::string& m_statusText;
        std::mutex& m_mutex;
        std::atomic<bool>& m_cancelRequested;
    };

    /**
     * @class async_task_manager
     * @brief Arka plan işçi iş parçacığı (worker thread) ve ana iş parçacığı geri çağrım
     * dağıtıcısını (main thread dispatcher) yöneten tekil (singleton) asenkron görev yöneticisi.
     */
    class async_task_manager {
    public:
        using TaskWorkFn = std::function<bool(task_reporter& reporter)>;
        using MainThreadCompleteFn = std::function<void(bool success, const std::string& message)>;

        /**
         * @brief Singleton örneğine küresel erişim sağlar.
         */
        static async_task_manager& instance();

        async_task_manager();
        ~async_task_manager();

        // Kopyalama ve taşıma engellendi
        async_task_manager(const async_task_manager&) = delete;
        async_task_manager& operator=(const async_task_manager&) = delete;

        /**
         * @brief Yeni bir görevi arka plan iş kuyruğuna ekler ve hemen döner.
         * @param name Görevin kullanıcı dostu adı (Örn: "Compiling Project Scripts").
         * @param workFn Arka planda çalışacak fonksiyon.
         * @param onCompleteMainThread Görev bittiğinde ana iş parçacığında güvenle çağrılacak geri çağrım.
         */
        void run_task(
            const std::string& name,
            TaskWorkFn workFn,
            MainThreadCompleteFn onCompleteMainThread = nullptr);

        /**
         * @brief Ana iş parçacığında (render döngüsünde) biriken geri çağrımları tüketir ve çalıştırır.
         * @note Bu fonksiyon mutlaka ana iş parçacığından (örn. Engine::update başlangıcında) çağrılmalıdır.
         */
        void poll_main_thread_callbacks();

        /**
         * @brief Şu anda çalışan veya kuyrukta bekleyen bir görev olup olmadığını bildirir.
         */
        [[nodiscard]] bool is_busy() const noexcept {
            return m_isBusy.load(std::memory_order_acquire);
        }

        /**
         * @brief Anlık görevin ilerleme oranını döndürür (0.0f..1.0f, veya -1.0f ise belirsiz/marquee).
         */
        [[nodiscard]] float get_progress() const noexcept {
            return m_currentProgress.load(std::memory_order_relaxed);
        }

        /**
         * @brief Anlık görevin adını iş parçacığı güvenli şekilde döndürür.
         */
        [[nodiscard]] std::string get_task_name() const;

        /**
         * @brief Anlık durum açıklamasını iş parçacığı güvenli şekilde döndürür.
         */
        [[nodiscard]] std::string get_status_text() const;

        /**
         * @brief Çalışan göreve iptal sinyali gönderir.
         */
        void cancel_current_task();

        /**
         * @brief Görev yöneticisini ve arka plan iş parçacığını temiz ve güvenli şekilde sonlandırır.
         */
        void shutdown();

    private:
        struct TaskItem {
            std::string name;
            TaskWorkFn workFn;
            MainThreadCompleteFn onComplete;
        };

        void worker_loop();

    private:
        mutable std::mutex m_taskMutex;
        std::condition_variable m_cv;
        std::queue<TaskItem> m_taskQueue;

        std::thread m_workerThread;
        std::atomic<bool> m_stopRequested{ false };
        std::atomic<bool> m_isBusy{ false };
        std::atomic<bool> m_cancelCurrent{ false };

        std::atomic<float> m_currentProgress{ -1.0f };
        std::string m_currentTaskName;
        std::string m_currentStatusText;

        mutable std::mutex m_callbackMutex;
        std::vector<std::function<void()>> m_mainThreadCallbacks;
    };

} // namespace core


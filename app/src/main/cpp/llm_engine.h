#ifndef LLM_ENGINE_H
#define LLM_ENGINE_H

#include <string>
#include <memory>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>
#include <queue>

namespace localai {

/**
 * LLM Engine interface for text generation
 * This provides an abstraction layer for different LLM backends
 */
class LLMEngine {
public:
    using TokenCallback = std::function<void(const std::string& token)>;
    
    virtual ~LLMEngine() = default;
    
    // Initialize the engine with a model file
    virtual bool initialize(const std::string& modelPath) = 0;
    
    // Generate text synchronously
    virtual std::string generate(const std::string& prompt, int maxLength = 512) = 0;
    
    // Generate text with streaming callback
    virtual void generateStream(
        const std::string& prompt,
        TokenCallback callback,
        int maxLength = 512
    ) = 0;
    
    // Stop current generation
    virtual void stop() = 0;
    
    // Check if engine is initialized
    virtual bool isInitialized() const = 0;
    
    // Get model info
    virtual std::string getModelInfo() const = 0;
    
    // Release resources
    virtual void release() = 0;
};

/**
 * Stream context for managing streaming generation
 */
struct StreamContext {
    std::queue<std::string> tokenQueue;
    std::mutex mutex;
    std::condition_variable cv;
    std::atomic<bool> isComplete{false};
    std::atomic<bool> shouldStop{false};
    
    void pushToken(const std::string& token) {
        std::lock_guard<std::mutex> lock(mutex);
        tokenQueue.push(token);
        cv.notify_one();
    }
    
    bool popToken(std::string& token, int timeoutMs = 100) {
        std::unique_lock<std::mutex> lock(mutex);
        if (cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), 
            [this] { return !tokenQueue.empty() || isComplete.load(); })) {
            if (!tokenQueue.empty()) {
                token = tokenQueue.front();
                tokenQueue.pop();
                return true;
            }
        }
        return false;
    }
    
    void complete() {
        std::lock_guard<std::mutex> lock(mutex);
        isComplete = true;
        cv.notify_all();
    }
    
    void stop() {
        shouldStop = true;
        complete();
    }
};

} // namespace localai

#endif // LLM_ENGINE_H

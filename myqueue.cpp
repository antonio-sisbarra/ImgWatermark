#include <deque>
#include <condition_variable>
#include <mutex>

template <typename T>
class myqueue{
    private:
        std::mutex mutex;
        std::condition_variable cond;
        std::deque<T> queue;

    public:
        myqueue(){}
        void push(T const& value){
            {
                std::unique_lock<std::mutex> lock(mutex);
                queue.push_back(value);
            }
            this->cond.notify_one();
        }

        T pop(){
            std::unique_lock<std::mutex> lock(this->mutex);
            this->cond.wait(lock, [=]{return !this->queue.empty();});
            T res(std::move(this->queue.front()));
            this->queue.pop_front();
            return res;
        }
};
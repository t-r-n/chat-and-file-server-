
#ifndef ILOVERS_THREAD_POOL_H
#define ILOVERS_THREAD_POOL_H

#if 0
//�÷�
int main()
try {
    ilovers::TaskExecutor executor{ 10 };

    std::future<void> ff = executor.commit(f);
    std::future<int> fg = executor.commit(G{});
    std::future<std::string> fh = executor.commit([]()->std::string { std::cout << "hello, h !" << std::endl; return "hello,fh !"; });

    executor.shutdown();

    ff.get();
    std::cout << fg.get() << " " << fh.get() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    executor.restart();    // ��������
    executor.commit(f).get();    //

    std::cout << "end..." << std::endl;
    return 0;
}
catch (std::exception& e) {
    std::cout << "some unhappy happened... " << e.what() << std::endl;
}

#endif


#include <iostream>
#include <functional>
#include <thread>
#include <condition_variable>
#include <future>
#include <atomic>
#include <vector>
#include <queue>

// �����ռ�
namespace ilovers {
    class TaskExecutor;
}

class ilovers::TaskExecutor {
    using Task = std::function<void()>;
private:
    // �̳߳�
    std::vector<std::thread> pool;
    // �������
    std::queue<Task> tasks;
    // ͬ��
    std::mutex m_task;
    std::condition_variable cv_task;
    // �Ƿ�ر��ύ
    std::atomic<bool> stop;
    //std::vector<std::atomic<bool>> tasking;
public:
    // ����

    TaskExecutor(size_t size = 4) : stop{ false } {
        size = size < 1 ? 1 : size;
        //tasking.resize(size, false);
        for (size_t i = 0; i < size; ++i) {
            pool.emplace_back(&TaskExecutor::schedual, this);    // push_back(std::thread{...})
            //tasking[i].store(false);
        }
    }
    void join() {//�ȴ���������ִ�н���
        /*���ÿ�ʼ������ǩ��1��ֹͣ�ύ����2�߳�����������ִ������˳�*/
        shutdown();//ֹͣ�����ύ
        //�������Ϊ�����̳߳����߳̾�����

        restart();//���������ύ
        std::cout << "join���߳�" << std::this_thread::get_id() << std::endl;
        while (true) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }
    // ����
    ~TaskExecutor() {
        for (std::thread& thread : pool) {
            thread.detach();    // ���̡߳���������
            //thread.join();        // �ȴ���������� ǰ�᣺�߳�һ����ִ����
        }
    }

    // ֹͣ�����ύ
    void shutdown() {
        this->stop.store(true);
    }

    // ���������ύ
    void restart() {
        this->stop.store(false);
    }

    // �ύһ������
    template<class F, class... Args>
    auto commit(F&& f, Args&&... args) ->std::future<decltype(f(args...))> {//������ֵ���ò������������Ĳ��������������Ĳ�������
        if (stop.load()) {    // stop == true ?
            throw std::runtime_error("task executor have closed commit.");
        }
        using ResType = decltype(f(args...));    // typename std::result_of<F(Args...)>::type, ���� f �ķ���ֵ����
        auto task = std::make_shared<std::packaged_task<ResType()>>(//����һ�����ð��κ���ʽ�ĺ�������װ����һ���޷���ֵ�޲����ĺ���������ͳһ����std::function<void()>
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)//ע���bind�Ľ��ʹ��
            ); 
        {    // ������񵽶���
            std::lock_guard<std::mutex> lock{ m_task };
            tasks.emplace([task]() {   // push(Task{...}),,����һ�����ð��κ���ʽ�ĺ�������װ����һ���޷���ֵ�޲����ĺ���������ͳһ����std::function<void()>
                (*task)();//��bind�Ѻ��������󶨺�Ͳ���Ҫ���õ�ʱ�򴫵ݲ�����,��lambda���ʽ����������ĳ���void()��ʽ����
                //std::cout << __LINE__ << std::endl;
                });
        }
        cv_task.notify_all();    // �����߳�ִ��

        std::future<ResType> future = task->get_future();//�첽�ĵȴ�packaged_task��װ����task�ķ���ֵ
        return future;
    }
    template<typename xClass, typename xReturn, typename...xParam>//������Ϳ����ػ���Ա�����汾��
    auto commit(xReturn(xClass::*&& pfn)(xParam...), xClass*&& pThis, xParam&&...lp) {
        return commit(std::bind(std::forward(pfn), std::forward(pThis), std::forward(lp...)));
    }

private:
    // ��ȡһ����ִ�е� task
    Task get_one_task() {//�����������ȥһ�����񽻸���������߳�ִ��
        std::unique_lock<std::mutex> lock{ m_task };
        cv_task.wait(lock, [this]() { return !tasks.empty(); });    // wait ֱ���� task
        Task task{ std::move(tasks.front()) };    // ȡһ�� task
        tasks.pop();
        return task;
    }

    // �������
    void schedual() {
        while (true) {
            //std::cout << __LINE__ << std::endl;
            if (Task task = get_one_task()) {
                
                task();    //

            }
            else {

            }
        }
    }
};

#endif
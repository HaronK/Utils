
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <cassert>
#include <random>
#include <atomic>
#include <memory>

using namespace std;

#include "queue.h"
#include "writer.h"
#include "reader.h"
#include "guard.h"

struct Data
{
    Data(int d) : next(nullptr), data(d) {}

    Data *next;
    int data;
};

using Queue = types::queue<Data>;

template<class T>
using Writer = types::writer<T>;

template<class T>
using Reader = types::reader<T>;

template<class T>
using GuardedWriter = types::guard<Writer<T>>;

template<class T>
using GuardedReader = types::guard<Reader<T>>;

std::random_device rd;
std::mt19937 gen(rd());

std::atomic<int> active_writers((0));

void writer_thread(int index, GuardedWriter<Queue> &q, int data_count, int max_sleep)
{
    int value = index * 1000;

    std::uniform_int_distribution<> dis(0, max_sleep);
//    std::uniform_int_distribution<> data_dis(1, data_count);
    auto cnt = data_count; //data_dis(gen);

    std::cout << "    [" << index << "] Writer start: " << cnt << " records.\n";

    for (auto i = 0; i < cnt; ++i)
    {
        Data *d = new Data(value);
        q->write(d);

        std::cout << "      [" << index << "] + " << value << "\n";
        std::cout.flush();

        value++;

        std::chrono::milliseconds ms(dis(gen));
        std::this_thread::sleep_for(ms);
    }

    auto aw = active_writers.fetch_sub(1);
    if (aw <= 1)
        q->set_writer_finished();

    std::cout << "    [" << index << "] Writer finish.\n";
}

void reader_thread(int index, GuardedReader<Queue> &q, int max_sleep)
{
    std::cout << "    [" << index << "] Reader start.\n";

    std::uniform_int_distribution<> dis(0, max_sleep);

    Data *d = nullptr;
    while (!q->is_writer_finished())
    {
        if (q->read(d))
        {
            if (d != nullptr)
            {
                std::cout << "      [" << index << "] - " << d->data << "\n";
                std::cout.flush();

                delete d;
                d = nullptr;
            }
            else
            {
                std::cout << "      [" << index << "] - E\n";
                std::cout.flush();
            }
        }

        std::chrono::milliseconds ms(dis(gen));
        std::this_thread::sleep_for(ms);
    }

    std::cout << "    [" << index << "] Reading tail...\n";
    std::cout.flush();

    while (q->read(d))
    {
        if (d != nullptr)
        {
            std::cout << "      [" << index << "] = " << d->data << "\n";
            std::cout.flush();

            delete d;
            d = nullptr;
        }
        else
        {
            std::cout << "      = E\n";
            std::cout.flush();
        }

        std::chrono::milliseconds ms(dis(gen));
        std::this_thread::sleep_for(ms);
    }

    std::cout << "    [" << index << "] Reader finish.\n";
}

class ThreadsBucket
{
public:
    ThreadsBucket(std::size_t count) : threads(count)
    {}

    template<class Function, class... Args>
    void init(Function&& f, Args&&... args)
    {
        for (std::size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].reset(new std::thread(f, i, std::forward<Args>(args)...));
        }
    }

    void join()
    {
        for (std::size_t i = 0; i < threads.size(); ++i)
        {
            threads[i]->join();
        }
    }

private:
    std::vector<std::unique_ptr<std::thread>> threads;
};

int main(int argc, const char* argv[])
{
    std::cout << "Start...\n";

    int attempts_count = 1, data_count = 20, w_count = 2, w_max_sleep = 100, r_count = 1, r_max_sleep = 300;

    if (argc == 7)
    {
        attempts_count = std::stoi(argv[1]);
        data_count     = std::stoi(argv[2]);
        w_count        = std::stoi(argv[3]);
        w_max_sleep    = std::stoi(argv[4]);
        r_count        = std::stoi(argv[5]);
        r_max_sleep    = std::stoi(argv[6]);
    }
    else if (argc != 1)
    {
        std::cout << "Usage: ./queue_multi_rw_test [<attempts_count:1> <data_count:20> " \
                     "<writers_count:2> <writer_max_sleep:100> <readers_count:1> <reader_max_sleep:300>]\n";
        return 0;
    }

    for (auto i = 0; i < attempts_count; ++i)
    {
        std::cout << "=======================================================\n";
        std::cout << "  Attempt " << i << "/" << attempts_count << "\n";

        auto q = std::make_shared<Queue>();
        GuardedWriter<Queue> gw(q);
        GuardedReader<Queue> gr(q);
        ThreadsBucket writers(w_count), readers(r_count);

        active_writers.store(w_count);

        writers.init(writer_thread, std::ref(gw), data_count, w_max_sleep);
        readers.init(reader_thread, std::ref(gr), r_max_sleep);

        writers.join();
        readers.join();
    }

    std::cout << "Finish.\n";

    return 0;
}

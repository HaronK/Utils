
#include <iostream>
#include <chrono>
#include <thread>
#include <future>
#include <atomic>
#include <cassert>
#include <random>
#include <memory>

using namespace std;

#include "queue.h"

struct Data
{
    Data(int d) : next(nullptr), data(d) {}

    Data *next;
    int data;
};

using Queue = types::queue<Data>;

std::random_device rd;
std::mt19937 gen(rd());

void writer_thread(std::shared_ptr<Queue> q, int n, int max_sleep)
{
    int value = 0;

    std::cout << "    Writer start...\n";

    std::uniform_int_distribution<> dis(0, max_sleep);

    for (auto i = 0; i < n; ++i)
    {
        Data *d = new Data(value);
        q->write(d);

        std::cout << "      + " << value << "\n";
        std::cout.flush();

        value++;

        std::chrono::milliseconds ms(dis(gen));
        std::this_thread::sleep_for(ms);
    }

    q->set_writer_finished();

    std::cout << "    Writer finish.\n";
}

void reader_thread(std::shared_ptr<Queue> q, int max_sleep)
{
    std::cout << "    Reader start...\n";

    std::uniform_int_distribution<> dis(0, max_sleep);

    Data *d = nullptr;
    while (!q->is_writer_finished())
    {
        if (q->read(d))
        {
            if (d != nullptr)
            {
                std::cout << "      - " << d->data << "\n";
                std::cout.flush();

                delete d;
                d = nullptr;
            }
            else
            {
                std::cout << "      - E\n";
                std::cout.flush();
            }
        }

        std::chrono::milliseconds ms(dis(gen));
        std::this_thread::sleep_for(ms);
    }

    std::cout << "    Reading tail...\n";
    std::cout.flush();

    while (q->read(d))
    {
        if (d != nullptr)
        {
            std::cout << "      - " << d->data << "\n";
            std::cout.flush();

            delete d;
            d = nullptr;
        }
        else
        {
            std::cout << "      - E\n";
            std::cout.flush();
        }
    }

    std::cout << "    Reader finish.\n";
}

int main(int argc, const char* argv[])
{
    std::cout << "Start...\n";

    int attempts_count = 1, data_count = 20, w_max_sleep = 100, r_max_sleep = 300;

    if (argc == 5)
    {
        attempts_count = std::stoi(argv[1]);
        data_count     = std::stoi(argv[2]);
        w_max_sleep    = std::stoi(argv[3]);
        r_max_sleep    = std::stoi(argv[4]);
    }
    else if (argc != 1)
    {
        std::cout << "Usage: ./queue_single_rw_test [<attempts_count:1> <data_count:20> <writer_max_sleep:100> <reader_max_sleep:300>]\n";
        return 0;
    }

    for (auto i = 0; i < attempts_count; ++i)
    {
        std::cout << "=======================================================\n";
        std::cout << "  Attempt " << i<< "/" << attempts_count << "\n";

        auto q = std::make_shared<Queue>();
        std::thread wt(writer_thread, q, data_count, w_max_sleep);
        std::thread rt(reader_thread, q, r_max_sleep);

        wt.join();
        rt.join();
    }

    std::cout << "Finish.\n";

    return 0;
}

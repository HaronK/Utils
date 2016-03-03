
#include <relacy/relacy.hpp>

using namespace rl;

#define assert RL_ASSERT

#include "queue.h"
#include "writer.h"
#include "reader.h"
#include "guard.h"

// ------------------------------------------------------------------

struct Data
{
    Data(int d) : data(d)
    {
        VAR(next) = nullptr;
    }
    int data;
    VAR_T(Data*) next;
};

std::ostream& operator<< (std::ostream& o, const Data* data)
{
    assert(data != nullptr);
    return o << data->data << " <" << (void*) data << ">:<" << (void*) data->VAR(next) << ">";
}

using Queue = types::queue<Data>;

template<class T>
using Writer = types::writer<T>;

template<class T>
using Reader = types::reader<T>;

template<class T>
using GuardedWriter = types::guard<Writer<T>>;

template<class T>
using GuardedReader = types::guard<Reader<T>>;

// ------------------------------------------------------------------

struct queue_single_rw_test: rl::test_suite<queue_single_rw_test, 2>
{
    Queue q;

    void thread(unsigned thread_index)
    {
        if (0 == thread_index)
        {
            auto data = new Queue::value_type(1);

            q.write(data);

            //q.set_writer_finished();
        }
        else
        {
            Queue::pointer data = nullptr;

            while (!q.read(data))
            {
            }

            RL_ASSERT(nullptr != data);
            RL_ASSERT(1 == data->data);

            delete data;
        }
    }
};

struct queue_multi_rw_test: rl::test_suite<queue_multi_rw_test, 3>
{
    int value = 0;

    std::shared_ptr<Queue> q = std::make_shared<Queue>();
    GuardedWriter<Queue> writer;
    GuardedReader<Queue> reader;

    queue_multi_rw_test() : writer(q), reader(q)
    {}

    void before()
    {
        std::cout << "+++++++++++++++++++++\n";
//        std::cout << "Writers : Readers\n";
//        for (int i = 0; i < params::static_thread_count; i += 2)
//            std::cout << i << "  ";
//        std::cout << "|  ";
//        for (int i = 1; i < params::static_thread_count; i += 2)
//            std::cout << i << "  ";
//        std::cout << "\n";
    }

    void thread(unsigned thread_index)
    {
//        std::cout << "Test: " << thread_index << "\n";

        if (0 == (thread_index & 1))
        {
            auto data = new Queue::value_type(value);
            auto flush = !writer->write(data);

            std::cout << "New[" << thread_index << "]: " << data << "\n";

            value++;

//            data = new Queue::value_type(value);
//            flush = !writer->write(data);
//
//            std::cout << "New[" << thread_index << "]: " << data << "\n";
//
//            value++;

//            for (int i = 0; i < (thread_index >> 1); ++i)
//                std::cout << "   ";
//            std::cout << (value - 1) << "\n";

//            writer.set_writer_finished();
        }
        else
        {
            Queue::pointer data = nullptr;

            while (!reader->read(data))
            {}

            RL_ASSERT(nullptr != data);

//            for (int i = 0; i < (params::static_thread_count >> 1); ++i)
//                std::cout << "   ";
//            std::cout << "      ";
//            for (int i = 0; i < (thread_index >> 1); ++i)
//                std::cout << "   ";
//
//            std::cout << data->data << "\n";

            std::cout << "Del[" << thread_index << "]: " << data << "\n";

            delete data;

            std::cout << "-----\n";
        }
    }
};

int main()
{
    rl::simulate<queue_single_rw_test>();
//    rl::simulate<queue_multi_rw_test>(); // TODO: fix test

    return 0;
}

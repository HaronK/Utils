/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Oleg Khryptul aka HaronK
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// NOTE: VAR_T and VAR macros are used for testing with
// Relacy Race Detector library:
// http://www.1024cores.net/home/relacy-race-detector/rrd-introduction
// http://www.1024cores.net/home/relacy-race-detector

#if !defined(VAR_T) || !defined(VAR)
#define VAR_T(t) t
#define VAR(v) v
#define VAR_UNDEF
#endif

namespace types
{

/**
 * Lock free queue for 1 writer and 1 reader threads.
 *
 * Writer and reader are working with a separate queues.
 * When writer writes to its own queue it checks if reader has anything to read.
 * If not then writer pass its queue to reader and starts new one for itself.
 *
 * The only place were reader and writer touch each other is when writer gives
 * its queue to reader via readerTop.
 *
 * If writer want to stop writing it should call set_writer_finished() method.
 * After that writer should not write anything to the queue otherwise behavior will be unpredictable.
 *
 * This class can be also used in multiple writer and/or reader configuration.
 * To do this one should use guard, writer and reader template classes. See test/queue_multi_rw_test.cpp
 * file for example.
 */
template<class T>
class queue
{
public:
    using value_type = T;
    using pointer    = T*;

    queue();
    ~queue();

    /**
     * Write data to the queue. Writer only method.
     * Return value can be used by writer to decide when flush() should be called.
     *
     * @param data Value to write to the queue
     * @return true if data was send to the reader otherwise false
     */
    bool write(pointer data);

    /**
     * Read data from queue. Reader only method.
     *
     * @param data [OUT] Data to retrieve.
     * @return true if data was retrieved otherwise false.
     */
    bool read(pointer &data);

    /**
     * Flush remained data to the reader. Writer only method.
     *
     * @return true if data was flushed otherwise false.
     */
    bool flush();

    void set_writer_finished()
    {
        VAR(writer_finished) = true;
    }

    bool is_writer_finished()
    {
        return VAR(writer_finished);
    }

private:
    std::atomic<pointer> reader_top;
    VAR_T(pointer) writer_top;
    VAR_T(pointer) writer_bottom;

    VAR_T(bool) writer_finished;
};

template<class T>
queue<T>::queue() : reader_top(nullptr)
{
    VAR(writer_top)      = nullptr;
    VAR(writer_bottom)   = nullptr;
    VAR(writer_finished) = false;
}

template<class T>
queue<T>::~queue()
{
    // clean reader's queue
    auto elem = reader_top.load(std::memory_order_acquire);
    while (elem != nullptr)
    {
        auto next = elem->VAR(next);
        delete elem;
        elem = next;
    }

    // clean writer's queue
    elem = VAR(writer_top);
    while (elem != nullptr)
    {
        auto next = elem->VAR(next);
        delete elem;
        elem = next;
    }
}

template<class T>
bool queue<T>::write(pointer data)
{
    assert(VAR(writer_finished) == false);
    assert(data != nullptr);

    data->VAR(next) = nullptr;

    if (VAR(writer_top) != nullptr)
    {
        VAR(writer_bottom)->VAR(next) = data; // append new element to the end of the writer's queue
    }
    else
    {
        VAR(writer_top) = data; // start new writer queue
    }
    VAR(writer_bottom) = data; // update pointer to the end of writer's queue

    if (reader_top.load(std::memory_order_acquire) == nullptr) // reader don't have anything to read
    {
        reader_top.store(VAR(writer_top), std::memory_order_release); // give reader writer's queue
        VAR(writer_top) = nullptr; // P1: start new writers queue
        return true;
    }
    return false;
}

template<class T>
bool queue<T>::read(pointer &data)
{
    // If writer stopped in write() method before command marked as P1 there could be 2 situations:
    // 1. If writer/reader threads/cpus became synchronized reader will not go inside a following 'if' and goes to the
    //    P2.
    // 2. Otherwise reader will go inside following 'if' and will always return false because isWriterFinished will
    //    always be false in this situation (see first assert in Write() method).
    VAR_T(pointer) val = reader_top.load(std::memory_order_acquire);
    if (VAR(val) == nullptr)
    {
        // Magic happens here. If writer is not finished writing and we have nothing to read then we go to the 'false'
        // branch and return false.
        // If writer is finished we go to the 'true' branch but we will not interfere in accessing to the 'readerTop' or
        // 'writerTop' variables with writer because he can not call Write() method after finishing.
        if (VAR(writer_finished) && VAR(writer_top) != nullptr)
        {
            // Reader will come at this place only when writer stops writing to the queue.
            // Reader just read remaining part of the data if present.
            reader_top.store(VAR(writer_top), std::memory_order_release);
            VAR(val) = reader_top.load(std::memory_order_acquire);
            VAR(writer_top) = nullptr;
        }
        else
        {
            return false;
        }
    }

    // P2
    // At this place we garantee that readerTop (val) variable is synchronized between reader and writer.
    // Also we can garantee here that readerTop != nullptr
    data = VAR(val);
    reader_top.store(VAR(val)->VAR(next), std::memory_order_release);
    data->VAR(next) = nullptr; // NOTE: this assigning to nullptr is not really necessary. Just to garantee that reader
                               // will not use 'next' pointer.

    return true;
}

/**
 * This method should be called by writer in a case when writer doesn't write into its own queue for a long time and
 * its queue is not empty. By calling this method writer gives reader remaining part of its queue.
 * Calling of this method by writer will not interfere with calling read() method by reader.
 */
template<class T>
bool queue<T>::flush()
{
    assert(VAR(writer_finished) == false);

    if (VAR(writer_top) == nullptr)
    {
        return true;
    }

    if (reader_top.load(std::memory_order_acquire) == nullptr)
    {
        reader_top.store(VAR(writer_top), std::memory_order_release);
        VAR(writer_top) = nullptr;
        return true;
    }
    return false;
}


} // namespace types

#ifdef VAR_UNDEF
#undef VAR_T
#undef VAR
#undef VAR_UNDEF
#endif

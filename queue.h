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
    
    void set_writer_finished()
    {
        VAR(writer_finished) = true;
    }

    bool is_writer_finished()
    {
        return VAR(writer_finished);
    }

private:
    atomic<pointer> writer_top;
    VAR_T(pointer) writer_bottom;

    VAR_T(bool) writer_finished;

    atomic<pointer> reader_top;
};

template<class T>
queue<T>::queue() : reader_top(nullptr)
{
    VAR(writer_top)      = nullptr;
    VAR(writer_bottom)   = nullptr;
    VAR(writer_finished) = false;
    VAR(reader_top)      = nullptr;
}

template<class T>
queue<T>::~queue()
{
    // clean reader's queue
    auto elem = reader_top.load(memory_order_acquire);
    while (elem != nullptr)
    {
        auto next = elem->VAR(next);
        delete elem;
        elem = next;
    }

    // clean writer's queue
    elem = writer_top.load(memory_order_acquire);
    while (elem != nullptr)
    {
        auto next = elem->VAR(next);
        delete elem;
        elem = next;
    }
}

/*
 * Write data to the queue.
 * Algorithm:
 * 1. Retrieve writer top using atomic::exchange(null).
 * 2. If it is null then create new item.
 * 3. Otherwise add data to the end.
 * 4. Retrieve reader top using atomic::load(null).
 * 5. If it is null then set it to the writer top.
 * 6. Otherwise restore writer's top.
 */
template<class T>
bool queue<T>::write(pointer data)
{
    assert(VAR(writer_finished) == false);
    assert(data != nullptr);

    data->VAR(next) = nullptr;

    VAR_T(pointer) w_top = writer_top.exchange(nullptr, memory_order_acq_rel);

    if (VAR(w_top) != nullptr)
    {
        VAR(writer_bottom)->VAR(next) = data; // append new element to the end of the writer's queue
    }
    else
    {
        VAR(w_top) = data; // start new writer queue
    }
    VAR(writer_bottom) = data; // update pointer to the end of writer's queue

    VAR_T(pointer) r_top = reader_top.load(memory_order_acquire);
    if (VAR(r_top) == nullptr) // reader don't have anything to read
    {
        reader_top.store(VAR(w_top), memory_order_release); // give reader writer's queue
        return true;
    }

    writer_top.store(VAR(w_top), memory_order_release); // restore writer's top

    return false;
}

/*
 * Read data from the queue.
 * Algorithm:
 * 1. Retrieve reader top using atomic::exchange(null).
 * 2. If it is null then:
 * 2.1. Retrieve writer top using atomic::exchange(null).
 * 2.2. If it is not null then assign it to the reader top.
 * 3. If reader top is not null return it and shift to the next.
 */
template<class T>
bool queue<T>::read(pointer &data)
{
    VAR_T(pointer) r_top = reader_top.exchange(nullptr, memory_order_acq_rel);
    if (VAR(r_top) == nullptr)
    {
        VAR(r_top) = writer_top.exchange(nullptr, memory_order_acq_rel);
        if (VAR(r_top) == nullptr)
        {
            return false;
        }
    }

    if (VAR(r_top)->VAR(next) != nullptr)
    {
        reader_top.store(VAR(r_top)->VAR(next), memory_order_release);
    }

    data = VAR(r_top);

    return true;
}

} // namespace types

#ifdef VAR_UNDEF
#undef VAR_T
#undef VAR
#undef VAR_UNDEF
#endif

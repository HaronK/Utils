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

/**
 * Lock free queue for 1 writer and 1 reader threads.
 * 
 * Writer and reader are working with a separate queues.
 * When writer writes data to its own queue it checks if reader has anything to read.
 * If not then writer pass its queue to reader and starts new one for itself.
 * 
 * The only place were reader and writer touch each other is when writer gives
 * its queue to reader but it's safe. For details look for comments in the code.
 * 
 * If writer want to stop writing it should call SetWriterFinished() method.
 * After that writer should not write anything to the queue otherwise behavior will be unpredictable.
 * 
 * TODO: this class can be refactored to support multiple writers and/or readers.
 * To do this 2 separate locks should be introduced for writers and readers.
 * In this case writer will block other writers but not readers and other way around for readers.
 * This will give lock independence between writers and readers.
 */
template <class T>
class LockFreeQueue
{
public:
    /**
     * Write data to the queue. Writer only method.
     * Return value can be used by writer to decide when Flush() should be called.
     * 
     * @param data Value to write to the queue
     * @return true if data was send to the reader otherwise false
     */
    bool Write(T *data);
    
    /**
     * Read data from queue. Reader only method.
     * 
     * @param data [OUT] Data to retrieve.
     * @return true if data was retrieved otherwise false.
     */
    bool Read(T *&data);

    /**
     * Flush remained data to the reader. Writer only method.
     * 
     * @return true if data was flushed otherwise false.
     */
    bool Flush();

    /**
     * Inform that writer is finished.
     */
    void SetWriterFinished() { isWriterFinished = true; }

    /**
     * Check if writer is finished.
     */
    bool IsWriterFinished() { return isWriterFinished; }

private:
    T *readerTop    = nullptr;
    T *writerTop    = nullptr;
    T *writerBottom = nullptr;

    bool isWriterFinished = false;
};

template<class T>
bool LockFreeQueue<T>::Write(T* data)
{
    assert(!isWriterFinished);
    assert(data != nullptr);

    data->next = nullptr;

    if (writerTop != nullptr)
    {
        writerBottom->next = data;
        writerBottom = data;
    }
    else
    {
        writerTop = writerBottom = data;
    }

    if (readerTop == nullptr) // reader don't have anything to read
    {
        readerTop = writerTop; // give reader writer's queue
        writerTop = nullptr; // P1: start new writers queue
        return true;
    }
    return false;
}

template<class T>
bool LockFreeQueue<T>::Read(T*& data)
{
    // If writer stoped in Write() method before command marked as P1 there could be 2 situations:
    // 1. If writer/reader threads/cpus became synchronized reader will not go inside a following 'if' and goes to the
    //    P2.
    // 2. Otherwise reader will go inside following 'if' and will always return false because isWriterFinished will
    //    always be false in this situation (see first assert in Write() method).
    if (readerTop == nullptr)
    {
        // Magic happens here. If writer is not finished writing and we have nothing to read then we go to the 'false'
        // branch and return false.
        // If writer is finished we go to the 'true' branch but we will not interfere in accessing to the 'readerTop' or
        // 'writerTop' variables with writer because he can not call Write() method after finishing.
        if (isWriterFinished && writerTop != nullptr)
        {
            // Reader will come at this place only when writer stops writing to the queue.
            // Reader just read remaining part of the data if present.
            readerTop = writerTop;
            writerTop = nullptr;

            if (readerTop == nullptr) // nothing to read
                return false;
        }
        else
        {
            return false;
        }
    }

    // P2
    // At this place we garantee that readerTop variable is synchronized between reader and writer.
    // Also we can garantee here that readerTop != nullptr
    data = readerTop;
    readerTop = data->next;

    return true;
}

/**
 * This method should be called by writer in a case when writer doesn't write into its own queue for a long time and
 * its queue is not empty. In this case reader will not receive data from writers queue.
 * Calling of this method by writer will not influence of calling Read() method by reader.
 */
template<class T>
bool LockFreeQueue<T>::Flush()
{
    assert(!isWriterFinished);

    if (writerTop == nullptr)
    {
        return true;
    }

    if (readerTop == nullptr)
    {
        readerTop = writerTop;
        writerTop = nullptr;
        return true;
    }
    return false;
}

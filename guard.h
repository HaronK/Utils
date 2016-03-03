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

// NOTE: $ macros is used for testing with
// Relacy Race Detector library:
// http://www.1024cores.net/home/relacy-race-detector/rrd-introduction
// http://www.1024cores.net/home/relacy-race-detector

#ifndef $
#define $
#define DOLLAR_UNDEF
#endif

namespace types
{

/**
 * Guard class that wraps each access to the enclosed object with mutex lock/unlock calls.
 */
template<class T, class Mutex = mutex>
class guard
{
    Mutex mutex;
    T obj;

public:
    template<class... Args>
    guard(Args&&... args) : obj(std::forward<Args>(args)...) {}

    class guard_ptr
    {
        guard *obj;

    public:
        explicit guard_ptr(guard *g) : obj(g)
        {
            obj->mutex.lock($);
        }

        ~guard_ptr()
        {
            obj->mutex.unlock($);
        }

        T* operator->()
        {
            return &obj->obj;
        }
    };

    guard_ptr operator->()
    {
        return guard_ptr(this);
    }
};

} // namespace types

#ifdef DOLLAR_UNDEF
#undef $
#undef DOLLAR_UNDEF
#endif

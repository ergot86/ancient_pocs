/*
 * smdetect.cpp
 *
 * Created by Daniel Fernandez (soyfeliz@48bits.com) on 19/03/2010
 *
 * Copyright (c) 2010  48BITS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the project's author nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <iostream>
#include <cstdlib>
#include <sys/time.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif

class Buffer
{
    int *buffer;
    unsigned int size;

    unsigned long long gettime()
    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        return ((unsigned long long) tv.tv_sec) * 1000000ULL +
                (unsigned long long) tv.tv_usec;
    }

public:

    enum Exceptions
    {
        ALLOC_FAILED,
        LOCK_FAILED,
    };

    Buffer(unsigned int sz, bool locking, unsigned int seconds) : size(sz)
    {
#ifdef _WIN32
        HANDLE hProcess(GetCurrentProcess());
        SIZE_T minws, maxws;

        buffer = (int *) VirtualAlloc(NULL, size, MEM_COMMIT, PAGE_READWRITE);

        if (buffer == NULL)
            throw ALLOC_FAILED;

        if (locking == true)
        {
            GetProcessWorkingSetSize(hProcess, &minws, &maxws);
            SetProcessWorkingSetSize(hProcess, minws + size, maxws + size);

            if (VirtualLock(buffer, size) == 0)
                throw LOCK_FAILED;
        }
#else
        buffer = (int *) mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE |
                              MAP_ANONYMOUS, 0, 0);

        if (buffer == MAP_FAILED)
            throw ALLOC_FAILED;

        if (locking == true && mlock(buffer, size) == -1)
            throw LOCK_FAILED;
#endif
        for (unsigned int x(0); x != size/sizeof(int); ++x)
            buffer[x] = 0x443d3d38;
#ifdef _WIN32
        Sleep(seconds * 1000);
#else
        sleep(seconds);
#endif
    }

    ~Buffer()
    {
#ifdef _WIN32
        VirtualUnlock(buffer, size);
        VirtualFree(buffer, 0, MEM_RELEASE);
#else
        munmap(buffer, size);
#endif
    }

    unsigned long long measureWrite()
    {
        unsigned long long start(gettime());

        for (unsigned int x(0); x != size/sizeof(int); x += 4096/sizeof(int))
            buffer[x] = x;

        return gettime() - start;
    }
};

unsigned long long getTime(unsigned int bsize, bool locking,
                           unsigned int sleep_seconds = 0)
{
    Buffer buf(bsize, locking, sleep_seconds);
    return buf.measureWrite();
}

int main(int argc, char *argv[])
{
    unsigned int buffer_size(15*1024*4096);
    unsigned int sleep_time(60);
    unsigned int threshold(500);
    bool locking(true);
    char c;
    
    while ((c = getopt(argc, argv, "pm:s:t:")) != -1)
    {
        switch(c)
        {
        case 'p':
            locking = false;
            break;
        case 'm':
            buffer_size = (atoi(optarg) * 1024 * 1024) & ~4095;
            break;
        case 's':
            sleep_time = atoi(optarg);
            break;
	case 't':
            threshold = atoi(optarg);
            break;
        case '?':
            std::cout << "\nOptions:\n"
                    << "-m size\t buffer size in Mb (default 60mb)\n"
                    << "-s time\t sleep time in second (default 60secs)\n"
                    << "-t num\t threshold (default 98)\n"
                    << "-p\t disable locking (default enabled)"
                    << std::endl;
            return -1;
        }
    }

    try
    {
        unsigned long long t1(getTime(buffer_size, locking));
        unsigned long long t2(getTime(buffer_size, locking, sleep_time));

	// avoid division by zero
	if (t1 == 0)
		t1 = 1;
	
        int variation((std::abs((long long)(t2 - t1))*100)/t1);

        std::cout << "First write:  " << t1
                << "\nSecond write: " << t2
                << "\nVariation:    " << variation << "%\n"
                << (variation >= threshold ? "VMM detected!" : "no VMM detected")
                << std::endl;
    }
    catch (Buffer::Exceptions e)
    {
        if (e == Buffer::ALLOC_FAILED)
        {
            std::cout << "Can't alloc buffer, try with a smaller size (-m)\n"
                    "Or check your capabilities" << std::endl;
        }

        if (e == Buffer::LOCK_FAILED)
        {
            std::cout << "Can't lock buffer, check your capabilities\n"
                    "Or use -p flag" << std::endl;
        }
    }

    return 0;
}

#pragma once

#if defined TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

#include <stdint.h>
#include <string.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

template <class T>
class Queue
{
    T* buffer = nullptr;
    int head  = 0, tail = 0, size = 0;

public:
    Queue() { }

    Queue(T* array, const int len)
    {
        size   = len;
        buffer = array;
    }

    void SetBuffer(T* array, const int len)
    {
        size   = len;
        head   = tail = 0;
        buffer = array;
    }

    int Get(T* elements, int len)
    {
        // Trim the length if necessary to only as large as the number of available elements in the buffer
        len = MIN(len, Avail());

        int nonwrapped    = MIN((size - tail), len);
        const int wrapped = len - nonwrapped;

        // memcpy the data starting at the head all the way up to the last element *(storage - 1)
        memcpy(elements, (buffer + tail), nonwrapped * sizeof(T));

        // If there's still data to copy memcpy whatever remains, starting at the first element *(begin) until the end of data. The first step will have ensured
        // that we don't crash into the tail during this process.
        memcpy((elements + nonwrapped), buffer, wrapped * sizeof(T));

        // Recalculate head
        tail = (tail + nonwrapped + wrapped) % size;

        return len;
    }

    // Returns the number of bytes actually placed in the array
    int Put(const T* elements, int len)
    {
        // Trim the length if necessary to only as large as the nuber of free elements in the buffer
        len = MIN(len, Free());

        // Figure out how much to append to the end of the buffer and how much will overlap onto the start
        int nonwrapped    = MIN((size - head), len);
        const int wrapped = len - nonwrapped;

        // memcpy the data starting at the head all the way up to the last element *(storage - 1)
        memcpy((buffer + head), elements, nonwrapped * sizeof(T));

        // If there's still data to copy memcpy whatever remains onto the beginning of the array
        memcpy(buffer, (elements + nonwrapped), wrapped * sizeof(T));

        // Re-recalculate head
        head = (head + nonwrapped + wrapped) % size;

        return len;
    }

    // Expand the size of queue without actually modifying any of the contents - useful for copying directly onto the queu buffer
    int Pad(int len)
    {
        // Trim the length if necessary to only as large as the nuber of free elements in the buffer
        len = MIN(len, Free());

        // Figure out how much to append to the end of the buffer and how much will overlap onto the start
        const int nonwrapped = MIN((size - head), len), wrapped = len - nonwrapped;

        // Re-recalculate head
        head = (head + nonwrapped + wrapped) % size;

        return len;
    }

    // Removes the oldest entry from the Queue
    void Pop() { if (Avail()) tail = (tail + 1) % size; }

    // Returns the oldest element in the array (the one added before any other)
    T& Tail() { return buffer[tail]; }

    // Returns the newest element in the array (the one added after every other)
    T& Head() { return buffer[(head + size - 1) % size]; }

    T& operator[](int n) { return buffer[tail + n % size]; }

    void Clear() { head = tail = 0; }

    int Avail() const { return (size + head - tail) % size; }

    int Free() const { return (size - 1 - Avail()); }

    // Gets the number of free elements that can be stored contiguously
    int FreeContiguous() { return head < tail ? tail - head - 1 : MIN(size - head, Free()); }

    // Gets a pointer to the next free address in the buffer
    T* NextFreeAddress() { return buffer + head; }
};

#endif // TARGET_HAS_ThirdPartyGUSBampCAPI_Linux

//CRingBuffer provided by g.tec
//website: http://www.gtec.at
//redistribution of this file has been offcially granted by g.tec

#pragma once

#include <algorithm> // std::min
#include <winnt.h>
#include <stdlib.h>

//#include <afxwin.h>

/*
 * Class representing a ring buffer with elements of type float.
 */
template <typename T>
class CRingBuffer
{
public:

    //Constructor. Creates an empty buffer with an initial capacity of zero.
    CRingBuffer() : _buffer(nullptr) { }

    //Destructor. Frees the allocated buffer.
    ~CRingBuffer()
    {
        if (_buffer != nullptr) { VirtualFree(_buffer, 0, MEM_RELEASE); }
        _buffer = nullptr;
    }

    /* 
     * Initializes the buffer with the specified capacity representing the number of elements that the buffer can contain.
     * Returns false if the memory couldn't be allocated (e.g. because of not enough free disk space); true, if the call succeeded.
     */
    bool Initialize(const uint32_t capacity)
    {
        //if the buffer has been allocated before, release this memory first
        if (_buffer != nullptr)
        {
            VirtualFree(_buffer, 0, MEM_RELEASE);
            _buffer = nullptr;
        }

        if (capacity > 0)
        {
            //allocate memory for the buffer
            _buffer = static_cast<T*>(VirtualAlloc(nullptr, capacity * sizeof(T), MEM_COMMIT, PAGE_READWRITE));

            //check if allocation succeeded
            if (_buffer == nullptr) { return false; }
            _capacity = capacity;
        }

        //reset the buffer positions
        Reset();

        return true;
    }

    //Clears the buffer by resetting both the start and end position to zero.
    void Reset()
    {
        _start   = 0;
        _end     = 0;
        _isEmpty = true;
    }

    //Returns the buffer's capacity it has been initialized to, i.e. the number of elements the buffer can contain.
    int GetCapacity() const { return _capacity; }

    //Returns the free space of the buffer, i.e. the number of new elements that can be enqueued before the buffer will overrun.
    int GetFreeSize() const { return _capacity - GetSize(); }

    //Returns the number of elements that the buffer currently contains (don't confuse the size (number of ACTUALLY contained elements) with the capacity (maximum number of elements that the buffer CAN contain)!).
    int GetSize() const
    {
        if (_isEmpty) { return 0; }
        if (_start < _end) { return _end - _start; }
        return _capacity - (_start - _end);
    }

    /*
     * Writes the specified number of elements from the specified source array into the ring buffer. If the number of elements to copy exceeds the free buffer space, only the free buffer space will be written, existing elements will NOT be overwritten.
     * float* source:       pointer to the first element of the source array whose elements should be stored into the ring buffer.
     * uint32_t length: the number of elements from the source array that should be copied into the ring buffer.
     */
    void Write(T* source, const uint32_t length)
    {
        //if buffer is full or no elements should be written, no elements can be written
        if ((!_isEmpty && _start == _end) || length <= 0) { return; }

        //if _start <= _end, split the free buffer space into two parts 
        uint32_t firstPartCapacity        = (_start <= _end) ? _capacity - _end : _start - _end;
        const uint32_t secondPartCapacity = (_start <= _end) ? _start : 0;

        //copy first part
        CopyMemory(&_buffer[_end], source, std::min(firstPartCapacity, length) * sizeof(T));

        //if a second part exists, copy second part
        if (length > firstPartCapacity)
        {
            CopyMemory(&_buffer[0], &source[firstPartCapacity], std::min(secondPartCapacity, length - firstPartCapacity) * sizeof(T));
        }

        //update buffer positions
        _end     = (_end + std::min(length, firstPartCapacity + secondPartCapacity)) % _capacity;
        _isEmpty = false;
    }

    /*
     * Copys the specified number of elements from the ring buffer into the specified destination array.
     * If there are less elements in the buffer than the to read, only available elements will be copied.
     * float *destination:  The array where to copy the elements from the ring buffer to.
     * uint32_t length: The number of elements to copy from the ring buffer into the destination array.
     */
    void Read(T* destination, const uint32_t length)
    {
        if (length <= 0) { return; }

        //if _start >= _end, split the read operation into two parts 
        int firstPartSize        = (_start < _end) ? std::min(length, _end - _start) : std::min(length, _capacity - _start);
        const int secondPartSize = (_start < _end) ? 0 : std::min(_end, length - firstPartSize);

        //copy first part
        CopyMemory(destination, &_buffer[_start], firstPartSize * sizeof(T));

        //if a second part exists, copy second part
        if (secondPartSize > 0) { CopyMemory(&destination[firstPartSize], &_buffer[0], secondPartSize * sizeof(T)); }

        //update the buffer positions
        _start = (_start + (firstPartSize + secondPartSize)) % _capacity;

        if (_start == _end) { _isEmpty = true; }
    }

protected:
    //the buffer array
    T* _buffer = nullptr;

    //the number of elements the buffer can contain
    uint32_t _capacity = 0;

    //the position of the first contained element of the buffer in the internal array
    uint32_t _start = 0;

    //the position of the first free element of the buffer in the internal array (this position - 1 equals the position of the last contained element of the buffer)
    uint32_t _end = 0;

    //flag indicating if the buffer is empty. Necessary because when _start == _end it is undefined if the buffer is full or empty.
    bool _isEmpty = true;
};

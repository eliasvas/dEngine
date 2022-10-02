#ifndef DARRAY_H
#define DARRAY_H

#include "dmem.h"

//pretty much this: https://github.com/niklas-ourmachinery/bitsquid-foundation/blob/master/array.h

template<typename T> struct dArray {
    dLinearAllocator *_allocator;
    u32 _size; //number of used elements
    u32 _capacity; //total number of elements
    T *_data;

	T &operator[](u32 i);
    //DOC: Initializes a Dynamic Array of type 'T' and 'size' elements
    void init(u32 size);
    //DOC: Clears the Array, along with the memory
    void clear(void);
    //DOC: Makes the Array's length equal to its capacity
    void trim(void);
    //DOC: Returns the number of elements in the Array
    u32 size(void);

    //DOC: pushes an element in the back of the Array
    void push_back(T v);
    //DOC: Pops the last element of the Array
    void pop_back(void);
};

template <typename T>
inline T& dArray<T>::operator[](uint32_t i)
{
    return this->_data[i];
}

template <typename T>
inline u32 dArray<T>::size(void){
    return this->_size;
}

extern dLinearAllocator scratch_alloc;

template <typename T>
void dArray<T>::init(u32 size){
    //this->_allocator.init(size * sizeof(T) * 32); //this is for test
    this->_allocator = &scratch_alloc;
    this->_size = 0;
    this->_capacity = 0;
    darray_set_capacity(this, size);
}

template <typename T>
void dArray<T>::clear(void){
    darray_resize(this, 0);
}

template <typename T>
void dArray<T>::trim(void){
    this->set_capacity(this->_size);
}

template<typename T> void darray_grow(dArray<T> *a, uint32_t min_capacity = 0){
    u32 new_capacity = a->_capacity*2 + 8;
    if (new_capacity < min_capacity)
        new_capacity = min_capacity;
    darray_set_capacity(a, new_capacity);
}

template <typename T> 
void darray_resize(dArray<T> *a, u32 new_size){
    if (new_size > a->_capacity)darray_grow(a, new_size);
    a->_size = new_size;
}

template<typename T> void darray_set_capacity(dArray<T> *a, uint32_t new_capacity){
    if (new_capacity == a->_capacity)
        return;

    if (new_capacity < a->_size)
        darray_resize(a, new_capacity);

    T *new_data = NULL;
    if (new_capacity > 0) {
        new_data = (T *)a->_allocator->allocAligned(sizeof(T)*new_capacity, alignof(T));
        memcpy(new_data, a->_data, sizeof(T)*a->_size);
    }

    //a->_allocator->free(a->_data);
    a->_data = new_data;
    a->_capacity = new_capacity;
}

template<typename T> inline void dArray<T>::push_back(T v){
    if (this->_size + 1 > this->_capacity)
        darray_grow(this);
    this->_data[this->_size++] = v;
}
template<typename T> inline void dArray<T>::pop_back(void){
    this->_size--;
}

#endif
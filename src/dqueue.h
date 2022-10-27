#ifndef DQUEUE_H
#define DQUEUE_H

#include "tools.h" //for typedefs
#include "dmem.h"
#include "darray.h"


template <typename T>
struct dQueue {
    dArray<T> data;
    u32 _size;
    u32 _offset;

    T &operator[](u32 i);

    void init(u32 cap = 32);
    void deinit(void);
    //DOC: the number of elements in the queue
    u32 size(void);
    //DOC: the amount of free space in the queue
    u32 space(void);
    //DOC: makes the queue be able to hold at least 'size' number of elements
    void reserve(u32 size);
    //DOC: appends item to the back of the queue
    void push_back(T item);
    //DOC: deletes the last element of the queue
    void pop_back(void);
    //DOC: appends item to the front of the queue
    void push_front(T item);
    //DOC: deletes the last element of the queue
    //DOC: pops the fron element
    void pop_front(void);

};

template <typename T>
void dQueue<T>::init(u32 cap){
    this->_offset = 0;
    this->_size = 0;
    this->data.init(cap);
}

template <typename T>
void dQueue<T>::deinit(void){
    this->_offset = 0;
    this->_size = 0;
    this->data.deinit();
}

template <typename T> inline T & dQueue<T>::operator[](uint32_t i)
{
    return this->data[(i + this->_offset) % this->data.size()];
}

template <typename T>
void dqueue_increase_cap(dQueue<T> *q, u32 new_cap){
    uint32_t end = q->data.size();
    q->data.resize(new_cap);
    if (q->_offset + q->_size > end) {
        uint32_t end_items = end - q->_offset;
        memmove(&q->data[0] + new_cap - end_items, &q->data[0] + q->_offset, end_items * sizeof(T));
        q->_offset += new_cap - end;
    }
}

template <typename T>
void dqueue_increase_grow(dQueue<T> *q, u32 min_cap = 0){
    // :/
    u32 new_cap = q->data.size()*2 + 8;
    if (new_cap < min_cap)
        new_cap = min_cap;
    dqueue_increase_cap(q, new_cap);
}

template <typename T>
u32 dQueue<T>::size(void){
    return this->_size;
}

template <typename T>
u32 dQueue<T>::space(void){
    return this->data.size() - this->_size;
}

template <typename T>
void dQueue<T>::reserve(u32 size){
    if (size > this->_size)
        dqueue_increase_cap(this, size);
}


template <typename T>
void dQueue<T>::push_back(T item){
    
    if (!this->space())
        dqueue_increase_grow(this);
    // ://////////////
    dQueue &q = *this;
    q[q._size++] = item;
}

template <typename T>
void dQueue<T>::pop_back(void){
    this->_size--;
}

template <typename T>
void dQueue<T>::pop_front(void){
    this->_offset = (this->_offset + 1) % this->data.size();
    --this->_size;
}

template <typename T>
void dQueue<T>::push_front(T item){
    
    if (!this->space())
        dqueue_increase_grow(this);
    // ://////////////
    this->_offset = (this->_offset - 1 + this->data.size()) % this->data.size();
    dQueue &q = *this;
    q[0] = item;
}
#endif
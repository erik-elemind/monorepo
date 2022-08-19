#ifndef _MIN_MAX_H_
#define _MIN_MAX_H_

#include "Deque.h"

// Implementation of "Ascending Minima"
// Original implementation:
// https://stackoverflow.com/questions/14823713/efficient-rolling-max-and-min-window
// Copied on: October 6th, 2020.

template<class T>
struct ExtremeValue
{
    unsigned long remove_index;
    T value;
    ExtremeValue(){ }
    ExtremeValue(size_t remove_index_, T value_): 
      remove_index(remove_index_), value(value_){
    }
};

template<class T, int MAX_WINDOW_SIZE>
class MovingMin{
private:
    Deque<ExtremeValue<T>,MAX_WINDOW_SIZE> queue;
    size_t win_size_;
    unsigned long index_;
    ExtremeValue<T> errVal;
public:
    MovingMin(size_t win_size): queue(win_size), win_size_(win_size), index_(0), errVal(0,0){
    }

    void setWindowSize(size_t win_size){
        // TODO: something more clever instead of resetting the queue
        queue.empty();
        queue.setSize(win_size);
        win_size_ = win_size;
        index_ = 0;
    }

    T getMin(){
        return queue.getFront(errVal).value;
    }

    T getMin(T val)
    {
        while (queue.getFill() > 0 && index_ >= queue.getFront(errVal).remove_index){
            queue.deleteFront();
        }

        while (queue.getFill() > 0 && queue.getBack(errVal).value >= val){
            queue.deleteBack();
        }

        ExtremeValue<T> newVal(index_ + win_size_, val);
        queue.insertBack(newVal);

        index_++;
        // TODO: something clever when index++ wraps around
        return queue.getFront(errVal).value;
    }
};

template<class T, int MAX_WINDOW_SIZE>
class MovingMax{
private:
    Deque<ExtremeValue<T>,MAX_WINDOW_SIZE> queue;
    size_t win_size_;
    unsigned long index_;
    ExtremeValue<T> errVal;
public:
    MovingMax(size_t win_size): queue(win_size), win_size_(win_size), index_(0), errVal(0,0){
    }

    void setWindowSize(size_t win_size){
        win_size_ = win_size;
    }

    T getMax(){
        return queue.getFront(errVal).value;
    }

    T getMax(T val)
    {
        while (queue.getFill() > 0 && index_ >= queue.getFront(errVal).remove_index){
            queue.deleteFront();
        }

        while (queue.getFill() > 0 && queue.getBack(errVal).value <= val){
            queue.deleteBack();
        }

        ExtremeValue<T> newVal(index_ + win_size_, val);
        queue.insertBack(newVal);

        index_++;
        // TODO: something clever when index++ wraps around
        return queue.getFront(errVal).value;
    }
};

#endif //_MIN_MAX_H_
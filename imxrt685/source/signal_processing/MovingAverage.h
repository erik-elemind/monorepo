#ifndef _MOVING_AVERAGE_H_
#define _MOVING_AVERAGE_H_

#include <string.h>

/**********************************************/
// STREAMLINED AVERAGES

template<typename T, typename TT, typename RT, int CAPACITY>
class MovingAverage {
  public:
    T window_[CAPACITY]; // the window array
    size_t size_; // the size of the window array
    size_t index_; // the index into the window array that will be updated next
    size_t fill_; // the number of real sample points in the window, so fill==0 means there are no samples.
    TT total_; // the total of everything in the array
    RT provided_avg_;
    MovingAverage(){
      resize(CAPACITY);
    }
    ~MovingAverage() {
    }
    inline void add(T val) {
      // insert the new value
      window_[index_] = val;
      // increment the index
      index_ = (index_+1) % size_;
      // increment the fill up to and including window size
      if (fill_ < size_) {
        fill_++;
      }
    }
    inline void resize(size_t size){
      size_ = size;
      index_ = 0;
      fill_ = 0;
      total_ = 0;
      memset(window_, 0, sizeof(window_)); // fill the window with zeros
    }
    /* 
    Returns the oldest values stored in the window.
    Assuming a window that stores SIZE number of samples:
    INDEX = 0, returns the oldest sample stored.
    INDEX = SIZE-1, returns the newest sample.
    */
    inline T get(size_t index) {
      return window_[(index_ + index) % size_];
    }
    /* 
    Returns the oldest values stored in the window.
    Assuming a window that stores MULT*SIZE number of samples,
    and MUTL_OFFSET = MULT - 1:
    INDEX = 0, returns the oldest sample stored.
    INDEX = SIZE-1, returns the newest sample.
    */
    inline T get_with_multiple(size_t index, size_t mult, size_t mult_offset) {
      return window_[(index_ + mult * index + mult_offset) % size_];
    }
    inline bool empty() {
      return fill_ == 0;
    }
    inline void reset() {
      memset(window_, 0, sizeof(window_)); // fill the window with zeros
      index_ = 0;
      fill_ = 0;
      total_ = 0;
    }
    inline size_t copyto(T* dest){
      memcpy(dest+fill_-index_, window_, sizeof(T)*index_);
      memcpy(dest, window_+index_, sizeof(T)*(fill_-index_));
      return fill_;
    }    
    RT average(T val) {
      // copy the old value and replace with new value
      T old_val = get(0);
      add(val);
      // incrementally update total
      total_ = total_ - old_val + val;
      // return average
      return (RT)total_ / fill_;
    }
    RT average() {
      return (RT)total_ / fill_;
    }
    bool full(){
      return fill_ == size_;
    }
    size_t getSize(){
      return size_;
    }
};


#endif  /* _MOVING_AVERAGE_H_ */


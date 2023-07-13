#ifndef _WINDOW_H_
#define _WINDOW_H_


/**********************************************/
// MOVING WINDOW

template<typename T, int CAPACITY>
class Window {
  public:
    T window_[CAPACITY]; // the window array
    size_t size_; // the size of the window array
    size_t index_; // the index into the window array that will be updated next
    size_t fill_; // the number of real sample points in the window, so fill==0 means there are no samples.
    Window(size_t size) {
      resize(size);
    }
    ~Window() {
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
    inline bool empty() {
      return fill_ == 0;
    }
    inline void reset() {
      memset(window_, 0, sizeof(window_)); // fill the window with zeros
      index_ = 0;
      fill_ = 0;
    }
    inline size_t copyto(T* dest){
      // void * memcpy ( void * destination, const void * source, size_t num );
      memcpy(dest+fill_-index_, window_, sizeof(T)*index_);
      memcpy(dest, window_+index_, sizeof(T)*(fill_-index_));
      return fill_;
    }    
    bool full(){
      return fill_ == size_;
    }
    size_t getSize(){
      return size_;
    }
};


#endif  /* _WINDOW_H_ */


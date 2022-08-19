// Implementation copied from:
// https://www.softwaretestinghelp.com/deque-in-cpp/
// Copied on: October 6th, 2020.
// Modified by David Wang

#ifndef _DEQUEUE_H_
#define _DEQUEUE_H_


// Deque class
template <class T, int MAX_CAPACITY>
class Deque
{
    T array[MAX_CAPACITY];
    int front;
    int rear;
    size_t size;
    size_t fill;
    const size_t cap;

public:
    Deque(size_t size) : cap(MAX_CAPACITY)
    {
        empty();
        setSize(size);
    }
    void empty(){
        front = -1;
        rear = 0;
        fill = 0;
    }
    void setSize(size_t size){
        if(size<=MAX_CAPACITY){
            this->size = size;
        }
    }
    // Operations on Deque:

    // Insert an element at front of the deque
    bool insertFront(T key)
    {
        if ( isFull() ) {
            return false;
        }

        // If queue is initially empty,set front=rear=0; start of deque
        if (front == -1) {
            front = 0;
            rear = 0;
        } else if (front == 0) { // front is first position of queue
            front = size - 1;
        } else { // decrement front 1 position
            front = front - 1;
        }
        array[front] = key; // insert current element into Deque
        fill++;
        return true;
    }

    // insert element at the rear end of deque
    bool insertBack(T key)
    {
        if (isFull()) {
            return false;
        }

        //  If queue is initially empty, set front=rear=0; start of deque
        if (front == -1) {
            front = 0;
            rear = 0;
        } else if (rear == (int)size - 1) { // rear is at last position of queue
            rear = 0;
        } else { // increment rear by 1 position
            rear = rear + 1;
        }
        array[rear] = key; // insert current element into Deque
        fill++;
        return true;
    }

    // Delete element at front of Deque
    bool deleteFront()
    {
        if ( isEmpty() ) {
            return false;
        }

        // Deque has only one element
        if (front == rear) {
            front = -1;
            rear = -1;
        } else if (front == (int)size - 1) {  // back to initial position
            front = 0;
        } else { // remove current front value from Deque;increment front by 1
            front = front + 1;
        }
        fill--;
        return true;
    }

    // Delete element at rear end of Deque
    bool deleteBack()
    {
        if (isEmpty()) {
            return false;
        }

        // Deque has only one element
        if (front == rear) {
            front = -1;
            rear = -1;
        } else if (rear == 0) {
            rear = size - 1;
        } else {
            rear = rear - 1;
        }
        fill--;
        return true;
    }
    // retrieve front element of Deque
    T getFront(T errVal)
    {
        if (isEmpty()) {
            return errVal;
        }
        return array[front];
    }

    // retrieve rear element of Deque
    T getBack(T errVal)
    {
        if (isEmpty() || rear < 0) {
            return errVal;
        }
        return array[rear];
    }
    // Check if Deque is full
    bool isFull()
    {
        return ((front == 0 && rear == (int)size - 1) || front == rear + 1);
    }
    // Check if Deque is empty
    bool isEmpty()
    {
        return (front == -1);
    }
    size_t getFill(){
        return fill;
    }
};


#endif //_DEQUEUE_H_

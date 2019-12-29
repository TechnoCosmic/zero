//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute	    Dirk Mahoney			dirk@tcri.com.au
// Catchpole Robotics					Christian Catchpole		christian@catchpole.net
//


#ifndef TCRI_ZERO_LIST_H
#define TCRI_ZERO_LIST_H


namespace zero {

    template <class T>
    class List {
    public:
        List();

        T* getHead();
        T* getTail();

        void prepend(T& item);
        void append(T& item);
        void remove(T& item);

    private:
        T* _head;
        T* _tail;
    };

}

#endif
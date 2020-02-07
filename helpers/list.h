//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_LIST_H
#define TCRI_ZERO_LIST_H


namespace zero {


    template <class T>
    class List {
    public:
        List();

        T* getHead() const;
        T* getTail() const;

        void prepend(T& item);
        void append(T& item);
        void remove(T& item);
        void insertBefore(T& item, T& before);

    private:
        T* _head;
        T* _tail;
    };


    template <class T>
    class OffsetList : public List<T> {
    public:
        void insertByOffset(T& item, const uint32_t offsetFromNow);
    };


}

#endif
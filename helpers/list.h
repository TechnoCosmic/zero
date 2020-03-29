//
// zero - pre-emptive multitasking kernel for AVR
//
// Techno Cosmic Research Institute    Dirk Mahoney           dirk@tcri.com.au
// Catchpole Robotics                  Christian Catchpole    christian@catchpole.net
//


#ifndef TCRI_ZERO_LIST_H
#define TCRI_ZERO_LIST_H


namespace zero {

    /// @brief A template class for doubly-linked lists
    template <class T>
    class List {
    public:
        List();

        T* getHead() const;
        T* getTail() const;

        void prepend( T& item );
        void append( T& item );
        void remove( T& item );
        void insertBefore( T& item, T& before );

    protected:
        T* _head;
        T* _tail;
    };


    /// @brief A template class for time ordered doubly-linked lists
    template <class T>
    class OffsetList : public List<T> {
    public:
        void remove( T& item );
        void insertByOffset( T& item, const uint32_t offsetFromNow );
    };

}    // namespace zero

#endif
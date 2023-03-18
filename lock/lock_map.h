//
// Created by shotacon on 23-3-17.
//

#ifndef YY_SERVER_LOCK_MAP_H
#define YY_SERVER_LOCK_MAP_H
#include "lock.h"

#include "map"

using namespace std;

template<class T, class U>
class Lock_Map{
public:
    void Insert(T t, U u){
        lock.lock();
        m[t] = u;
        lock.unlock();
    }

    U Find(T t){
        U u;
        lock.lock();
        if(m.count(t))
            u = m[t];
        lock.unlock();
        return u;
    }

    void Del(T t){
        lock.lock();
        if(m.count(t))
            m.erase(t);
        lock.unlock();
    }

private:
    map<T, U> m;
    locker lock;
};

#endif //YY_SERVER_LOCK_MAP_H

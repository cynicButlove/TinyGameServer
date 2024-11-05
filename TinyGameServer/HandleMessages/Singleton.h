//
// Created by zhangshiping on 24-11-5.
//

#ifndef TINYGAMESERVER_SINGLETON_H
#define TINYGAMESERVER_SINGLETON_H

template <typename T>
class Singleton {
public:
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static T& Instance() {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    ~Singleton() = default;
};



#endif //TINYGAMESERVER_SINGLETON_H

#include "thread_pool/thread_pool.h"

void* fun(void *){
    std::cout<<"thread work"<<std::endl;

    return nullptr;
}

int main() {
    Thread_Pool pool(10, 5, 10);

    for(int i = 0; i < 5; ++i){
        pool.Producer(fun, nullptr);
    }

    getchar();
    return 0;
}

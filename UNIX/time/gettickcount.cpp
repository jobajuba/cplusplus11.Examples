#include<iostream>
#include<ctime>
#include<unistd.h>
#include<sys/time.h>

uint32_t getTickCount() {
    timeval val;
    gettimeofday(&val, 0);
    uint32_t t = (val.tv_sec * 1000) + (val.tv_usec / 1000);
    return t;
}


int main(int argc, char **argv) {
    
    unsigned long stop = 0, diff = 0;
    std::cout << "Sleeping for 1 second..\n";
    unsigned long start = getTickCount();
    sleep(1);
    stop = getTickCount();
    diff = stop-start;
    std::cout << "Amount of ticks: " << diff << "\n";
    return EXIT_SUCCESS;
}

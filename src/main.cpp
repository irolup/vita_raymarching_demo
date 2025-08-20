#include <stdio.h>
#include "AppController.h"

#ifndef LINUX_BUILD
#include <psp2/kernel/processmgr.h>
#endif

int main(int argc, char** argv) {
#ifdef LINUX_BUILD
    printf("Starting PSVita RayMarching - Linux Build...\n");
#else
    printf("Starting PSVita RayMarching...\n");
#endif
    
    AppController app;
    
    if (!app.initialize()) {
        printf("Failed to initialize application\n");
#ifdef LINUX_BUILD
        return 1;
#else
        sceKernelExitProcess(1);
        return 1;
#endif
    }
    
    printf("Application initialized successfully\n");
    
    app.run();
    
    printf("Application finished\n");
    
#ifdef LINUX_BUILD
    return 0;
#else
    sceKernelExitProcess(0);
    return 0;
#endif
}

#undef __cplusplus // we are not using C++

#include "memflow_win32.h"
#include <stdio.h>
#include <zconf.h>
#include <math.h> //fmod
#include <pthread.h>
#include <sys/time.h>

#include "sigscanner.h"
#include "inputsystem.h"
#include "egggame.h"
#include "peeper/client/peeper.h"

#define EXE_NAME "diabotical.exe"
#define DLL_NAME "diabotical.exe"

static bool running = true;
static uint8_t localPlayerBytes[0x1000];
static uint8_t localEntityBytes[0x1000];
static uint8_t viewMatrixBytes[0x1000];

long long current_timestamp() {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

int main(int argc, char *argv[]) {
    log_init(1);

    Kernel                  *kernel = 0;
    Win32Process            *egggame = 0;
    Win32ModuleInfo         *module = 0;
    Win32ProcessInfo        *procInfo = 0;
    OsProcessInfoObj        *osProcInfo = 0;
    OsProcessModuleInfoObj  *osModuleInfo = 0;
    Address                 moduleBase;
    Address                 moduleSize;
    VirtualMemoryObj        *mem = 0;
    int                     threadErr;
    pthread_t               inputThread;

    threadErr = pthread_create(&inputThread, NULL, RunInputSystem, &running );
    if( threadErr )
    {
        printf("Failed to create input thread!\n");
        return -2;
    }

    if( Open() )
    {
        printf("Peeper open failed!\n");
        return -1;
    }

    ConnectorInventory *inv = inventory_try_new();
    CloneablePhysicalMemoryObj *conn = inventory_create_connector(inv, "qemu_procfs", "");


    if (!conn)
    {
        printf("Couldn't open conn!\n");
        inventory_free(inv);
        return 1;
    }

    kernel = kernel_build(conn);
    if( !kernel )
    {
        connector_free(conn);
        inventory_free(inv);
        return 2;
    }

    egggame = kernel_into_process(kernel, EXE_NAME);
    if( !egggame )
    {
        printf("Couldn't find diabotical.exe\n");
        kernel_free(kernel);
        inventory_free(inv);
        connector_free(conn);
        return 3;
    }

    module = process_module_info(egggame, DLL_NAME);
    if( !module )
    {
        printf("Couldn't find module %s\n", DLL_NAME);
        process_free(egggame);
        kernel_free(kernel);
        inventory_free(inv);
        connector_free(conn);
        return 4;
    }

    osModuleInfo = module_info_trait(module);
    moduleBase = os_process_module_base(osModuleInfo);
    moduleSize = os_process_module_size(osModuleInfo);
    mem = process_virt_mem(egggame);

    printf("module %s - base(%p) - size(%p)\n", DLL_NAME, moduleBase, moduleSize);
    uint8_t header[256];
    if (!virt_read_raw_into(mem, moduleBase, header, 256)) {
        printf("Read successful!\n");
        for (int o = 0; o < 8; o++) {
            for (int i = 0; i < 32; i++) {
                printf("%2hhx ", header[o * 32 + i]);
            }
            printf("\n");
        }
    } else {
        printf("Failed to read!\n");
    }

    // LocalPlayer (LocalEntity)
    // xref "Demo time: " to a function.
    // shortly below it, there should be
    // mov rax, cs:localEntity
    // test rax, rax
    Address localPlayerAddr = FindPatternInMemory( mem, "75 ? 48 8B 05 ? ? ? ? 48 85 C0 74 ? F2", moduleBase, moduleSize ) + 2;
    localPlayerAddr = GetAbsoluteAddressVm( mem, localPlayerAddr, 3, 7 );
    localPlayerAddr = virt_read_u64( mem, localPlayerAddr );
    // Local Entity, bit different from player, think of it just like your entity entry in the list.
    Address localEntityAddr = FindPatternInMemory( mem, "48 8B 05 ? ? ? ? 8B 88 ? ? ? ? 89 4C 24 50", moduleBase, moduleSize );
    localEntityAddr = GetAbsoluteAddressVm( mem, localEntityAddr, 3, 7 );
    localEntityAddr = virt_read_u64( mem, localEntityAddr );

    Address entityListAddr = FindPatternInMemory( mem, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 10", moduleBase, moduleSize );
    entityListAddr = GetAbsoluteAddressVm( mem, entityListAddr, 3, 7 );
    entityListAddr = virt_read_u64( mem, entityListAddr );

    Address viewMatrixAddr = FindPatternInMemory( mem, "48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8B 15 ? ? ? ? 48 81 C2", moduleBase, moduleSize );
    viewMatrixAddr = GetAbsoluteAddressVm( mem, viewMatrixAddr, 3, 7 );

    if( !localPlayerAddr || !localEntityAddr )
    {
        printf("Couldn't find localplayer sig!\n");
        goto end;
    }

    printf( "localPlayer @ (%p) - linux(%p)\n", (void*)localPlayerAddr, (void*)localPlayerBytes);
    printf( "localEntity @ (%p) - linux(%p)\n", (void*)localEntityAddr, (void*)localEntityBytes);
    printf( "viewmatrix @ (%p) - linux(%p)\n", (void*)viewMatrixAddr, (void*)viewMatrixBytes);

    struct LocalPlayer localPlayer;
    struct Entity localEntity;
    struct ViewMatrix viewMatrix;

    bool writePlayer = false;
    bool shouldHover = false;
    double yaw, pitch;
    long long lastToggle = current_timestamp();

    printf("Starting main loop...\n");
    while( running )
    {
        virt_read_raw_into(mem, localPlayerAddr, localPlayerBytes, 0x1000);
        virt_read_raw_into(mem, localEntityAddr, localEntityBytes, 0x1000);
        virt_read_raw_into(mem, viewMatrixAddr, viewMatrixBytes, 0x1000);

        virt_read_raw_into(mem, localPlayerAddr, (uint8_t*)&localPlayer, sizeof(struct LocalPlayer));
        virt_read_raw_into(mem, localEntityAddr, (uint8_t*)&localEntity, sizeof(struct Entity));
        virt_read_raw_into(mem, viewMatrixAddr, (uint8_t*)&viewMatrix, sizeof(struct ViewMatrix));

        //localEntity.posY += 0.001f;
        //virt_write_raw( mem, localPlayerAddr, (uint8_t*)&localPlayer, sizeof(struct LocalPlayer));
        //printf("LocalPlayer: Pos(%f/%f/%f) - Angle(%f/%f)\n", localEntity.posX, localEntity.posY, localEntity.posZ,
        //        RAD2DEG(localPlayer.pitch), RAD2DEG(fmodf( localPlayer.yaw, (M_PI * 2.0f) )) - 180.0f );

        pitch = localPlayer.pitch;
        yaw = fmod( localPlayer.yaw, (M_PI * 2.0f) );

        // iterate entities and do various hacks
        IterateEntities( mem, entityListAddr, &viewMatrix );
        // Submit any draw requests these functions may have made.
        SubmitDraws();

        if( pressedKeys[KEY_SPACE] )
        {
            localPlayer.velocityY += 1.25f;
            localPlayer.velocity2Y += 1.25f;
            writePlayer = true;
        }

        if( pressedKeys[KEY_LEFTCTRL] && (current_timestamp() - lastToggle) > 250 )
        {
            printf("toggling hover\n");
            shouldHover = !shouldHover;
            lastToggle = current_timestamp();
        }

        if( shouldHover )
        {
            localPlayer.velocityY = 0.0;
            localPlayer.velocity2Y = 0.0;
            writePlayer = true;
        }

        if( writePlayer )
            virt_write_raw( mem, localPlayerAddr, (uint8_t*)&localPlayer, sizeof(struct LocalPlayer));

        usleep( 1000 );
    }



end:
    printf("jiaa shutting down...\n");

    running = false;
    pthread_join( inputThread, NULL );
    ClearDraws();
    Close();
    if( osProcInfo )
        os_process_info_free(osProcInfo);
    if( osModuleInfo )
        os_process_module_free(osModuleInfo);
    if( kernel )
        kernel_free(kernel);
    if( inv )
        inventory_free(inv);

    return 0;
}


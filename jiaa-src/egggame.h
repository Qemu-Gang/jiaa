#ifndef JIAA_EGGGAME_H
#define JIAA_EGGGAME_H

#include "math.h"

#include <stdint.h>
#include <stdbool.h>
#include <memflow.h>

enum
{
    WEAPON_MELEE = 0,
    WEAPON_MG,
    WEAPON_PLASMA,
    WEAPON_SHOTGUN,
    WEAPON_ROCKETLAUNCHER,
    WEAPON_SHAFT,
    WEAPON_CROSSBOW,
    WEAPON_RAILGUN,
    WEAPON_GRENADELAUNCHER = 8,
    // ???
    WEAPON_HEALING_BALL = 12,
};

struct Entity
{
    //name at around 0x470, read until nul byte.
    char _pad[0x6B8];
    double posX;
    double posY; //vertical
    double posZ;
    char _pad2[64];
    int health;
    int armor;
    char _pad3[8];
    int currentGun;
};

// credits: zZzeta/S - https://www.unknowncheats.me/forum/other-fps-games/415582-diabotical-release-reversal-discussion.html
struct __attribute__((__packed__)) CEntTypeNeutral
{
	struct CEntTypeNeutral* type2_child; //0x0000
    struct CEntTypeNeutral* type1_ents; //0x0008
    struct CEntTypeNeutral* type2_entry; //0x0010
	char pad00[1]; //0x0018
	uint8_t last_entry; //0x0019
	char pad02[14]; //0x001A
	struct Entity* entity; //0x0028
}; //Size: 0x001A

//different from an entity.
struct LocalPlayer
{
        double localPosX; //0x0000
        double localPosY; //0x0008
        double localPosZ; //0x0010
        char pad_0018[96]; //0x0018
        double velocityX; //0x0078
        double velocityY; //0x0080
        double velocityZ; //0x0088
        char pad_0090[104]; //0x0090
        double velocity2X; //0x00F8
        double velocity2Y; //0x0100
        double velocity2Z; //0x0108
        char pad_0110[112]; //0x0110
        double N00000080; //0x0180
        char pad_0188[48]; //0x018
        // These are both in radians
        // yaw is SPECIAL, it does not reset when you reach the end of your sweep.
        // Instead it will keep going ( I got it in the hundreds )
        // to get the angle, modulo by 2*PI
        double yaw; //0x01B0
        // -pi/2 = all the way up
        // pi/2 = all the way down.
        double pitch; //0x01B8
        double yaw2; //0x01C0
        double pitch2; //0x01C8
        char pad_01D0[88]; //0x01D0
        double secondsAlive; //0x0228
        char pad_0230[80]; //0x0230
}; //Size: 0x0280

//// Functions
bool world_to_screen( const struct ViewMatrix *viewmatrix, const struct Vector *world, struct Vector2D *screenOut );

void IterateEntities( VirtualMemoryObj *memory, uintptr_t entityListAddr, const struct ViewMatrix *viewMatrix );

#endif //JIAA_EGGGAME_H

#include "egggame.h"

#include "peeper/client/peeper.h"
#include <stdio.h>

bool world_to_screen( const struct ViewMatrix *viewmatrix, const struct Vector *world, struct Vector2D *screenOut )
{
    struct ViewMatrix matrix = MatrixTranspose( viewmatrix );

    const struct Vector translation = { matrix.m[3][0], matrix.m[3][1], matrix.m[3][2] };
    const struct Vector up = { matrix.m[1][0], matrix.m[1][1], matrix.m[1][2] };
    const struct Vector right = { matrix.m[0][0], matrix.m[0][1], matrix.m[0][2] };

    float w = VectorDotProduct( &translation, world ) + matrix.m[3][3];

    // behind us
    if( w < 0.1f )
        return false;

    float x = VectorDotProduct( &right, world ) + matrix.m[0][3];
    float y = VectorDotProduct( &up, world ) + matrix.m[1][3];

    screenOut->x = ((1920.0f * 0.5f) * (1.0f + (x/w)));
    screenOut->y = ((1080.0f * 0.5f) * (1.0f - (y/w)));

    if( screenOut->x > 1920 || screenOut->x < 0 || screenOut->y > 1080 || screenOut->y < 0 )
        return false;

    return true;
}

// The entityListAddr passed into here is the one that starts with 3 pointers
// credits: zZzeta/S - https://www.unknowncheats.me/forum/other-fps-games/415582-diabotical-release-reversal-discussion.html
void IterateEntities( VirtualMemoryObj *memory, uintptr_t entityListAddr, const struct ViewMatrix *viewMatrix )
{
    struct CEntTypeNeutral entity_list2;
    virt_read_raw_into( memory, entityListAddr, (uint8_t*)&entity_list2, sizeof(struct CEntTypeNeutral));

    if( !entity_list2.type1_ents )
        return;

    struct CEntTypeNeutral entity_list;

    virt_read_raw_into( memory, (Address)entity_list2.type1_ents, (uint8_t*)&entity_list, sizeof(struct  CEntTypeNeutral));

    struct CEntTypeNeutral m;

    //printf("=======Entities======\n");
    do
    {
        struct Entity entity;

        if( !entity_list.last_entry && entity_list.entity )
        {
            virt_read_raw_into( memory, (Address)entity_list.entity, (uint8_t*)&entity, sizeof(struct Entity) );
            struct Vector pos;
            pos.x = (float)entity.posX;
            pos.y = (float)entity.posY;
            pos.z = (float)entity.posZ;
            struct Vector2D screen;
            struct Color color = { 225, 0, 0, 255 };
            if( world_to_screen( viewMatrix, &pos, &screen ) )
            {
                //printf("egg on screen(%f/%f)\n", screen.x, screen.y);
                AddCircle( screen.x, screen.y, color, 5.0f, 16, 2.0f );
                AddLine( 1920/2, 1080/2, screen.x, screen.y, color, 2.0f );
            }
        }

        // goto next entry
        struct CEntTypeNeutral type2_entry;
        virt_read_raw_into( memory, (Address)entity_list.type2_entry, (uint8_t*)&type2_entry, sizeof(struct CEntTypeNeutral) );

        if( type2_entry.last_entry )
        {
            for(
                    virt_read_raw_into( memory, (Address)entity_list.type1_ents, (uint8_t*)&m, sizeof(struct CEntTypeNeutral) );
                    !m.last_entry;
                // set m to m->type1_ents
                    virt_read_raw_into( memory, (Address)m.type1_ents, (uint8_t*)&m, sizeof(struct CEntTypeNeutral) )
                    )
            {
                //if( entity_list_addr != m.type2_entry )
                struct CEntTypeNeutral temp;
                virt_read_raw_into( memory, (Address)m.type2_entry, (uint8_t*)&temp, sizeof(struct CEntTypeNeutral) );
                if( entity_list.entity != temp.entity )
                    break;

                entity_list = m;
            }
            entity_list = m;
        }
        else
        {
            // set entity_list = entity_list->type2_entry
            virt_read_raw_into( memory, (Address)entity_list.type2_entry, (uint8_t*)&entity_list, sizeof(struct CEntTypeNeutral) );

            struct CEntTypeNeutral n;
            for (
                    virt_read_raw_into( memory, (Address)type2_entry.type2_child, (uint8_t*)&n, sizeof(struct CEntTypeNeutral) );
                    !n.last_entry;
                // set n to n->type2_child
                    virt_read_raw_into( memory, (Address)n.type2_child, (uint8_t*)&n, sizeof(struct CEntTypeNeutral) )
                    )
            {
                entity_list = n;
            }
        }
    }while( !entity_list.last_entry );

    //printf("=====================\n");
}
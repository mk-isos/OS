#include <stdio.h>
#include <stdint.h>
#include "smalloc.h"

int main()
{
    void *p1, *p2, *p3, *p4, *p5;

    smmode mode;

    for (int i = 0; i < 3; i++)
    {
        if (i == 0)
        {
            mode = bestfit;
            printf("Testing bestfit\n");
        }
        else if (i == 1)
        {
            mode = worstfit;
            printf("Testing worstFit\n");
        }
        else if (i == 2)
        {
            mode = firstfit;
            printf("Testing firstFit\n");
        }

        p1 = smalloc_mode(800, mode);
        printf("insert p1 (800 size)\n");
        p2 = smalloc_mode(1500, mode);
        printf("insert p2 (1500 size)\n");
        p3 = smalloc_mode(500, mode);
        printf("insert p3 (500 size)\n");
        p4 = smalloc_mode(10, mode);
        printf("insert p4 (10 size)\n");

        printf("Allocations\n");
        smdump();

        sfree(p1);
        sfree(p3);
        printf("Memory after freeing p1 (800 size), p3 (500 size)\n");
        smdump();

        // Allocate a block that best fits into the freed space
        p5 = smalloc_mode(30, mode);
        printf("insert p5 (30 size)\n");
        smdump();

        sfree(p5);
        printf("Memory after freeing p5 (30 size) with smcoalesce()\n");
        smcoalesce();
        smdump();

        // reallocation test

        printf("srealloc (p4, 1000)\n");
        printf("then the space of p4 increases from 10 to 1000\n");
        srealloc(p4, 1000);
        smdump();

        p5 = smalloc_mode(30, mode);
        printf("insert p5 (30 size)\n");
        smdump();

        // Reset the memory for next test
        sfree(p2);
        sfree(p4);
        sfree(p5);
        smcoalesce();
    }

    printf("\n");
    printf("===smcoalesce() test to use sfree()===\n");

    p1 = smalloc_mode(500, mode);
    printf("insert p1 (500 size)\n");
    p2 = smalloc_mode(200, mode);
    printf("insert p2 (200 size)\n");
    p3 = smalloc_mode(300, mode);
    printf("insert p3 (300 size)\n");
    p4 = smalloc_mode(100, mode);
    printf("insert p4 (100 size)\n");
    p5 = smalloc_mode(200, mode);
    printf("insert p5 (200 size)\n");
    smdump();

    sfree(p5);
    printf("Memory after freeing p5 (200 size)\n");
    smdump();
    smcoalesce();
    printf("after smcoalesce()\n");
    smdump();

    sfree(p3);
    printf("Memory after freeing p3 (300 size)\n");
    smdump();
    smcoalesce();
    printf("after smcoalesce()\n");
    smdump();

    sfree(p2);
    printf("Memory after freeing p2 (200 size)\n");
    smdump();
    smcoalesce();
    printf("after smcoalesce()\n");
    smdump();

    sfree(p1);
    sfree(p4);
    smcoalesce();

    return 0;
}
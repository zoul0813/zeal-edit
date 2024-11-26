#include <stdlib.h>
#include <stdio.h>
#include <zos_errors.h>
#include <zos_time.h>
#include <zvb_gfx.h>
// #include <unistd.h> // for sleep()
#include "conio.h"

#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 40

// // Define colors
// #define BLACK   0
// #define BLUE    1
// #define GREEN   2
// #define CYAN    3
// #define RED     4
// #define MAGENTA 5
// #define YELLOW  6
// #define WHITE   7

#define FIRE_COLORS 8


// Fire simulation constants
#define FIRE_WIDTH SCREEN_WIDTH
#define FIRE_HEIGHT SCREEN_HEIGHT - 2

unsigned char fire[FIRE_WIDTH * FIRE_HEIGHT];

// Function to initialize the fire array
void initFire(void) {
    for (int i = 0; i < FIRE_WIDTH * FIRE_HEIGHT; i++) {
        fire[i] = 0; // Set all to 0, which means no fire
    }
}

// Function to update the fire simulation
void updateFire(void) {
    // Loop through the fire array from the bottom up
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int y = SCREEN_HEIGHT - 1; y > 0; y--) {
            int currentIndex = y * SCREEN_WIDTH + x;
            int belowIndex = (y + 1) * SCREEN_WIDTH + x;

            // The fire intensity of the current cell is set to the intensity of the cell below, but with some randomness to simulate cooling
            fire[currentIndex] = fire[belowIndex] - (rand() % 3);  // Fire cools down as it rises
            // if (fire[currentIndex] < 0) {
            //     fire[currentIndex] = 0; // Prevent negative intensity values
            // }
        }
    }

    // Randomly generate fire intensity at the bottom row
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        fire[(SCREEN_HEIGHT - 1) * SCREEN_WIDTH + x] = rand() % FIRE_COLORS; // Random intensity for the bottom row
    }
}

// Function to draw the fire on the screen
void drawFire(void) {
    gfx_wait_vblank(NULL);
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            int intensity = fire[y * SCREEN_WIDTH + x];

            // Set color based on fire intensity
            textcolor(intensity);
            cputcxy(x, y, 176);
        }
    }
    gfx_wait_end_vblank(NULL);
}

// int main(void) {
//     // Initialize the graphics
//     clrscr();
//     cursor(0);
//     initFire();

//     while (1) {
//         updateFire();
//         drawFire();
//         // msleep(50); // Sleep for 50ms to slow down the animation
//     }

//     return 0;
// }

#include <stdbool.h>
#include <stdlib.h>

volatile int pixel_buffer_start; // global variable


void swap(int *num1, int *num2);
void plot_pixel(int x, int y, short int line_color);
void clear_screen();
void wait_for_vsync();
void draw_line(int x0, int y0, int x1, int y1, short int colour);
void draw_tile(int x0, int y0);
int* random_column();

//generates a list from [0]->[99] of random numbers between 0-3
int* random_column(){
    static int array[100];
    for (int i=0; i<100; i++){
        array[i] = rand() % 4;
        //printf("%d",array[i]);
    }
    return array;
}

int main(void) {
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    int N = 4;
    int dx_box[N], dy_box[N], x_box[N], y_box[N];
    int* tiles_position = random_column();
    
    //Prepare tiles position
    for (int i = 0; i < N; i++) {
        dy_box[i] = 5;

        x_box[i] = 80+i*40;
    }

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
    pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer

    while (1) {
        clear_screen();

        for (int i = 0; i < N; i++) {
            //draws the tile in black
            draw_tile(x_box[i], y_box[i]);
            y_box[i] += dy_box[i];

        }
    
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

//draws a box centered at coordinates
void draw_box(int x0, int y0) {
    int xsize = 40;
    int ysize = 60;
    for (int x = x0; x <= x0 + xsize; x++) {
        for (int y = y0; y <= y0 + ysize; y++) {
            plot_pixel(x, y, 0x0000);
        }
    }
}

//draws the piano tile where specified in black, give start coordinates of top left
//and ensure the size doesn't go off screen on the bottom
void draw_tile(int x0, int y0) {
    int xsize = 40;
    int ysize = 60;
    if(y0>=180){
        ysize = 240-y0;
    }

    for (int x = x0; x <= xsize+x0 ; x++) {
        for (int y = y0; y <= ysize+y0; y++) {
            plot_pixel(x, y, 0x0000);
        }
    }
}

//draws the pixel at a specified coordinate on the VGA display
void plot_pixel(int x, int y, short int line_color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

//clears the screen by drawing a pixel at every spot in white
void clear_screen() {
    for(int x=0; x < 320; x++) {
        for(int y=0; y < 240; y++) {
            plot_pixel(x, y, 0xffff);
        }
    }
}

void wait_for_vsync() {
    volatile int * pixel_ctrl_ptr = 0xFF203020; //pixel controller
    register int status;

    *pixel_ctrl_ptr = 1; //start the sync process

    status = *(pixel_ctrl_ptr + 3);
    //wait for the S bit to become 0
    while ((status &0x01) !=0) {
        status = *(pixel_ctrl_ptr + 3);
    }
}

// Swaps the values of 2 variables
void swap(int *num1, int *num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

//uses Bresenham's algorithm to determine where each pixel goes to draw a line
void draw_line(int x0, int y0, int x1, int y1, short int colour) {
    //if the change in y is greater than the change in x
    bool steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep) {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    int deltax = x1 - x0;
    int deltay = abs(y1 - y0);
    int error = -(deltax / 2);
    int y = y0;
    int y_step;

    if (y0 < y1) {
        y_step = 1;
    }

    else {
        y_step = -1;
    }

    for(int x = x0; x<= x1; x++) {
        if (steep) {
            plot_pixel(y,x, colour);
        }

        else {
            plot_pixel(x, y, colour);
        }

        error += deltay;

        if (error >= 0) {
            y += y_step;
            error -= deltax;
        }
    }
}
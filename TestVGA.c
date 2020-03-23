#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

volatile int pixel_buffer_start; // global variable

void clear_screen();
void draw_line(int x1, int y1, int x2, int y2, short int line_color);
void draw_pixel(int x, int y, short int line_color);
void swap(int* a, int* b);
void wait_for_vsync();
void draw_box(int x_box,int y_box,int short color_box);


int main(void)
{
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
    // declare other variables(not shown)
    // initialize location and direction of rectangles(not shown)

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

    //Set up the database of directions and colors
    int N = 5;
    int color_box[N], dx_box[N], dy_box[N], x_box[N], y_box[N];
    short color[5] = {0xFFFF,0xF800,0x07E0,0x001F,0xF81F};
    for(int i = 0; i < N; i++){
        dx_box[i] = rand()%2*2-1;
        dy_box[i]=rand()%2*2-1;
        color_box[i] = color[rand()%5];
        x_box[i] = rand()%320;
        y_box[i] = rand()%240;
    }

    while (1)
    {
        clear_screen();
        for(int i = 0; i < N; i++){
            draw_box(x_box[i],y_box[i],color_box[i]);
            draw_line(x_box[i],y_box[i],x_box[(i+1)%N],y_box[(i+1)%N], color_box[i]);
            
            //Set direction for bounce effect
            if(x_box[i]==319){
                dx_box[i] = -1;
            } else if (x_box[i]<=5){ //5 so drawing of box doesnt go negative
                dx_box[i] = 1;
            }
            if(y_box[i]==239){
                dy_box[i] = -1;
            } else if (y_box[i]<=5){ //5 so drawing of box doesnt go negative
                dy_box[i] = 1;
            }

            //Move box
            x_box[i] = x_box[i]+dx_box[i];
            y_box[i] = y_box[i]+dy_box[i];
        }
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
    }
}

void draw_box(int x0, int y0, int short box_color){
    int box_size = 6;
    for(int x = x0-box_size/2; x <= x0+box_size/2; x++){
        for(int y = y0-box_size/2; y <= y0+box_size/2; y++){
            draw_pixel(x,y, box_color);
        }
    }
}

void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = 0xFF203020; //pixel controller
    register int status;

    *pixel_ctrl_ptr = 1; //start sync process
    
    //Wait for S to become 0
    status = *(pixel_ctrl_ptr + 3);
    while((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }

}

void clear_screen(){
    int xMax = 319;
    int yMax = 239;
    for(int x = 0; x <= xMax; x++){
        for(int y = 0; y <= yMax; y++){
            draw_pixel(x,y, 0x0000);
        }
    }
}

// code not shown for clear_screen() and draw_line() subroutines
void draw_line(int x0, int y0, int x1, int y1, short int line_color) {
    bool is_steep = abs(y1 - y0) > abs(x1 - x0);
    if (is_steep){
        swap(&x0,&y0);
        swap(&x1,&y1);
    }
    if (x0 > x1){
        swap(&x0,&x1);
        swap(&y0,&y1);
    }
    
    int deltaX = x1-x0;
    int deltaY = abs(y1-y0);
    int error = -(deltaX/2);
    int y = y0;
    int y_step;
    if (y0 < y1){
        y_step = 1;
    } else {
        y_step = -1;
    }
    
    for(int x = x0; x <= x1; x++){
        if(is_steep){
            draw_pixel(y,x,line_color);
        } else {
            draw_pixel(x,y,line_color);
        }
        error = error + deltaY;
        if (error >= 0){
            y = y + y_step;
            error = error - deltaX;
        }
    }
}

void swap(int* a, int* b){
    int temp;
    temp = *b;
    *b = *a;
    *a = temp;
}

void draw_pixel(int x, int y, short int line_color) {
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}


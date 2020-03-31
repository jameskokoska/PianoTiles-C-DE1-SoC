#include <stdbool.h>
#include <stdlib.h>

volatile int pixelBufferStart; // global variable


void swap(int *num1, int *num2);
void plotPixel(int x, int y, short int line_color);
void clearScreen();
void waitForVsync();
void drawLine(int x0, int y0, int x1, int y1, short int colour);
void drawTile(int x0, int y0);
int* randomColumn();

//generates a list from [0]->[99] of random numbers between 0-3
int* randomColumn(){
    static int array[100];
    for (int i=0; i<100; i++){
        array[i] = rand() % 4;
        //printf("%d",array[i]);
    }
    return array;
}

int main(void) {
    volatile int * pixelCtrlPtr = (int *)0xFF203020;
    int N = 4;
    int DXBox[N], DYBox[N], xBox[N], yBox[N];
    int* tilesPosition = randomColumn();
    int currentTile = 0;
    
    //Prepare tiles position
    for (int i = 0; i < N; i++) {
        DYBox[i] = 10;

        xBox[i] = 80+tilesPosition[currentTile+i]*40;
        yBox[i] = abs(60*(i-(N-1)));
    }

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixelCtrlPtr + 1) = 0xC8000000; // first store the address in the 
                                        // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    waitForVsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixelBufferStart = *pixelCtrlPtr;
    clearScreen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixelCtrlPtr + 1) = 0xC0000000;
    pixelBufferStart = *(pixelCtrlPtr + 1); // we draw on the back buffer

    //Main game loop
    while (1) {

        

        /*---------Draw Stuffs------------*/
        clearScreen();
        for (int i = 0; i < N; i++) {
            //draws the tile in black
            drawTile(xBox[i], yBox[i]);
            yBox[i] += DYBox[i];
        }
        
        

        waitForVsync(); // swap front and back buffers on VGA vertical sync
        pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

        if(yBox[0] >= 240){
            currentTile++;
            for (int i = 0; i < N; i++) {
                DYBox[i] = 5;

                xBox[i] = 80+tilesPosition[currentTile+i]*40;
                yBox[i] = abs(60*(i-(N-1)));
            }
        }
    }
}


//draws the piano tile where specified in black, give start coordinates of top left
//and ensure the size doesn't go off screen on the bottom
void drawTile(int x0, int y0) {
    int xSize = 40;
    int ySize = 60;
    if(y0>=180){
        ySize = 240-y0;
    }

    for (int x = x0; x <= xSize+x0 ; x++) {
        for (int y = y0; y <= ySize+y0; y++) {
            plotPixel(x, y, 0x0000);
        }
    }
}

//draws a box centered at coordinates
void drawBox(int x0, int y0) {
    int xSize = 40;
    int ySize = 60;
    //This does center tile
    for (int x = x0 - xSize/2; x <= x0 + xSize/2; x++) {
        for (int y = y0 - ySize/2; y <= y0 + ySize/2; y++) {
            plotPixel(x, y, 0x0000);
        }
    }
}

//draws the pixel at a specified coordinate on the VGA display
void plotPixel(int x, int y, short int lineColor) {
    *(short int *)(pixelBufferStart + (y << 10) + (x << 1)) = lineColor;
}

//clears the screen by drawing a pixel at every spot in white
void clearScreen() {
    for(int x=0; x < 320; x++) {
        for(int y=0; y < 240; y++) {
            plotPixel(x, y, 0xffff);
        }
    }
}

void waitForVsync() {
    volatile int * pixelCtrlPtr = 0xFF203020; //pixel controller
    register int status;

    *pixelCtrlPtr = 1; //start the sync process

    status = *(pixelCtrlPtr + 3);
    //wait for the S bit to become 0
    while ((status &0x01) !=0) {
        status = *(pixelCtrlPtr + 3);
    }
}

// Swaps the values of 2 variables
void swap(int *num1, int *num2) {
    int temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

//uses Bresenham's algorithm to determine where each pixel goes to draw a line
void drawLine(int x0, int y0, int x1, int y1, short int colour) {
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
    int yStep;

    if (y0 < y1) {
        yStep = 1;
    }

    else {
        yStep = -1;
    }

    for(int x = x0; x<= x1; x++) {
        if (steep) {
            plotPixel(y,x, colour);
        }

        else {
            plotPixel(x, y, colour);
        }

        error += deltay;

        if (error >= 0) {
            y += yStep;
            error -= deltax;
        }
    }
}
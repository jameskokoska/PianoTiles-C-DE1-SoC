unsigned int titlePage[] = {};

unsigned int endPage[] = {};

unsigned int highscorePage[] = {};
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

volatile int pixelBufferStart; // global variable
volatile int * KEYPointer = (int *) 0xFF200050; //points to the KEYS on the board
volatile int * KeyboardPointer = (int *) 0xFF200100; //reads data for the ps2 keyboard
volatile int * pixelCtrlPtr = (int *)0xFF203020;
volatile int * a9TimerPtr = (int *) 0xFFFEC600;
volatile int * HEXptr = (int *) 0xFF200020;

void swap(int *num1, int *num2);
void plotPixel(int x, int y, short int line_color);
void clearScreen();
void waitForVsync();
void drawBox(int x0, int y0, int xSize, int ySize, short int color);
void drawLine(int x0, int y0, int x1, int y1, short int colour);
void drawTitlePage();
void drawEndPage();
void drawHighscorePage();
void drawTile(int x0, int y0, short int color);
int* randomColumn();
int checkTile (int keyPushed, int frontTile, int gamemode, int frontTilesSurprise);
void drawStatus (int x0, int y0, bool correct, int keyPushedStore);
void drawText(int x, int y, char * textPtr);
void drawSegNum(int number, int deltaX, int deltaY, short int color);
void animateHighscore(int x0, int y0, int xSize, int ySize);
void drawTileClear(int x0, int y0, int dy);
int timerCheck();


int widthGlobal = 320;
int heightGlobal = 240;

//generates a list from [0]->[99] of random numbers between 0-3
int* randomColumn(){
    static int array[100];
    for (int i=0; i<100; i++){
        array[i] = rand() % 4;
    }
    return array;
}

//generates a list from [0]->[99] of random numbers between 0-3
int* randomSurprise(){
    static int array[100];
    for (int i=0; i<100; i++){
        if(rand() % 2 == 1){  
            array[i] = rand() % 4+1;
        } else {
            array[i] = 0;
        }
        printf("%d",array[i]);
    }
    return array;
}

int main(void) {
    int highscores[] = {0,0,0,0,0}; 
    char seg7[] = {0b00111111, 0b00000110, 0b01011011, 0b01001111, 0b01100110, 0b01101101, 0b01111101, 0b00000111, 0b01111111, 0b01100111};
    int N = 4;
    int xBox[N], yBox[N];
    int DYBox = 0;
    int* tilesPosition = randomColumn();
    int* tilesSurprise = randomSurprise();
    int currentTile = 0;
    int keyPushedStore = 0;
    int keyPushed = 0;
    int correct = 0;
    int score = 0;
    bool gameEnd = false;
    bool firstPress = true;
    bool highscore = false;
    int animateX = 0;
    int animateY = 0;
    bool gameOver = false;
    int clearOnce = 0; //clears 2 buffers
    bool animateTile = false;
    int timer = 99;
    int animate = 0;
    bool checkTimer = true;
    *a9TimerPtr = 200000000; //sets up the timer to have a 1s delay
    *(a9TimerPtr + 2) = 0b011; // sets I,A, and E bits in the timer
    int animateTileCount;
    int gamemode = 2;
    bool gameOverWait = false;
    bool startGame = true;

    //Main game loop
    while (1) {
        DYBox = 0;
        tilesPosition = randomColumn();
        tilesSurprise = randomSurprise();
        currentTile = 0;
        keyPushedStore = 0;
        keyPushed = 0;
        correct = 0;
        score = 0;
        gameEnd = false;
        firstPress = true;
        highscore = false;
        animateX = 0;
        animateY = 0;
        gameOver = false;
        clearOnce = 0; //clears 2 buffers
        animateTile = false;
        startGame = true;
        timer = 99;
        
        animate = 0;
        animateTileCount = 0;
        gameOverWait = false;
        //Prepare tiles position
        for (int i = 0; i < N; i++) {
            xBox[i] = 80+tilesPosition[currentTile+i]*40;
            yBox[i] = abs(60*(i-(N-1)));
        }

        /* set front pixel buffer to start of FPGA On-chip memory */
        *(pixelCtrlPtr + 1) = 0xC8000000; // first store the address in the 
                                            // back buffer
        
        /* initialize a pointer to the pixel buffer, used by drawing functions */

        pixelBufferStart = *pixelCtrlPtr;
        clearScreen(); // pixel_buffer_start points to the pixel buffer
        drawTitlePage();
        /* now, swap the front/back buffers, to set the front buffer location */
        waitForVsync();
        

        //clears the existing inputs in the FIFO by reading them
        for(int i = 0; i < 3; i++) {
            keyPushed = *KeyboardPointer;
        }
        

        /* set back pixel buffer to start of SDRAM memory */
        *(pixelCtrlPtr + 1) = 0xC0000000;
        pixelBufferStart = *(pixelCtrlPtr + 1); // we draw on the back buffer

        drawTitlePage();
        waitForVsync();
        pixelBufferStart = *(pixelCtrlPtr + 1);
        

        //Clear the text
        char textScoreName[40] ="              \0";
        drawText(3, 3, textScoreName); 

        while(!gameOver) {
            int fBit = 0;
            checkTimer = true;
            while (fBit == 0) {
                fBit = *(a9TimerPtr + 3);

                if (timer <= 0) {
                    gameEnd = true;
                    timer = 0;
                }

                if (animate > 3) {
                    keyPushed = 0;
                }
                

                while((keyPushed != 0x8015 && keyPushed != 0x801d && keyPushed != 0x8024 && keyPushed != 0x802d && keyPushed != 0x8016 && keyPushed != 0x801e) && !gameEnd) {
                    keyPushed = *KeyboardPointer;
                    if(animate > 3) {
                         //clears the existing inputs in the FIFO by reading them
                        for(int i = 0; i < 3; i++) {
                            keyPushed = *KeyboardPointer;
                        }
                        if(timerCheck() == 1) {
                            break;
                        }
                        *HEXptr = seg7[timer % 10] | seg7[timer/ 10] << 8;
                        timer --;
                    }
                    if (gamemode == 2 && tilesSurprise[currentTile] == 1){
                        break;
                    }
                }
                
                while(keyPushedStore == 0 && firstPress != true && gameEnd==false){
                    keyPushed = *KeyboardPointer; 
                    correct = checkTile(keyPushed, tilesPosition[currentTile], gamemode, tilesSurprise[currentTile]);
                    animate = 0;
                    if(correct==1) {
                        keyPushedStore = 1;
                    } else if (correct==-1){
                        gameEnd = true;
                        drawText(25, 58, " \0");
                        drawText(25+10*1, 58, " \0");
                        drawText(25+10*2, 58, " \0");
                        drawText(25+10*3, 58, " \0");
                        break;
                    } else if (gamemode == 2 && correct == 2 && tilesSurprise[currentTile]==4){
                        keyPushedStore = 1;
                        score++;
                    }
                    if (tilesSurprise[currentTile]==1 && gamemode == 2){
                        DYBox = 60;
                    } else if(keyPushedStore){
                        DYBox = 10;
                    }
                }

                if(yBox[0] >= heightGlobal){
                    currentTile++;
                    score++;
                    keyPushedStore = 0;
                    DYBox = 0;
                    for (int i = 0; i < N; i++) {
                        xBox[i] = 80+tilesPosition[currentTile+i]*40;
                        yBox[i] = abs(60*(i-(N-1)));
                    }
                }

                //Wait for user to press one key to start
                if(firstPress){
                    for(int i = 0; i < 10; i++) {
                        keyPushed = *KeyboardPointer;
                    }
                }
                while(firstPress == true){
                    keyPushed = *KeyboardPointer;
                    if(startGame && keyPushed == 0x8016){
                        gamemode = 1;
                        startGame = false;
                        firstPress = false;
                        timer = 60;
                    } else if (startGame && keyPushed == 0x801e){
                        gamemode = 2;
                        startGame = false;
                        firstPress = false;
                        timer = 30;
                    } else if (!startGame && (keyPushed == 0x8015 || keyPushed == 0x801d || keyPushed == 0x8024 || keyPushed == 0x802d)){
                        firstPress = false;
                    } 
                    if (gameOverWait == true){
                        gameOver = true;
                    }
                }
                if(gameOver)
                    break;

                /*=========================================================*/
                /*=                   All Drawing Below                   =*/
                /*=========================================================*/
                
                

                if(clearOnce<2){
                    clearScreen();
                    clearOnce++;
                    animateTile = true;
                } else if (gamemode==2&&!gameEnd&&!animateTile){
                    drawBox(80, 240-60,40*4,60,0xFFFF);
                } else if (!gameEnd&&!animateTile){
                    drawBox(80, 240-20,40*4,20,0xFFFF);
                }


                if(!gameEnd){
                    char textScore[40];
                    sprintf(textScore, "%d", score); 
                    char textScoreName[40] ="Score: ";
                    drawText(3, 3, strcat(textScoreName, textScore)); 

                    //Text background
                    drawBox(10,10,39,6,0xF800);
                    
                    if(gamemode == 2 && tilesSurprise[currentTile] == 3){
                        drawText(25, 58, "R\0");
                        drawText(25+10*1, 58, "E\0");
                        drawText(25+10*2, 58, "W\0");
                        drawText(25+10*3, 58, "Q\0");
                    } else if(gamemode == 2 && tilesSurprise[currentTile] == 2){
                        drawText(25, 58, " \0");
                        drawText(25+10*1, 58, " \0");
                        drawText(25+10*2, 58, " \0");
                        drawText(25+10*3, 58, " \0");
                    } else {
                        drawText(25, 58, "Q\0");
                        drawText(25+10*1, 58, "W\0");
                        drawText(25+10*2, 58, "E\0");
                        drawText(25+10*3, 58, "R\0");
                    }
                    

                    for (int j = 0; j < N; j++) {
                        //draws the tile in black
                        
                        if(gamemode == 2 && tilesSurprise[currentTile+j]==1)
                            drawTile(xBox[j], yBox[j], 0x8FEA);
                        else if(gamemode == 2 && tilesSurprise[currentTile+j]==2)
                            drawTile(xBox[j], yBox[j], 0x52DF);
                        else if(gamemode == 2 && tilesSurprise[currentTile+j]==3)
                            drawTile(xBox[j], yBox[j], 0xD197);
                        else if(gamemode == 2 && tilesSurprise[currentTile+j]==4)
                            drawTile(xBox[j], yBox[j], 0xFC40);
                        else if (gamemode == 1 || tilesSurprise[currentTile+j]==0)
                            drawTile(xBox[j], yBox[j], 0x0000);

                        yBox[j] += DYBox;
                    }
                    animateTile = true;
                    if(animateTileCount == 4){
                        animateTile = false;
                        animateTileCount = 0;
                    } else if (tilesSurprise[currentTile]==1 && gamemode == 2){
                        animateTile = false;
                    }
                    animateTileCount++;
                    animate++;
                    //draws red or green based on if the user input was correct
                    drawStatus(xBox[0], yBox[0] - DYBox, correct, keyPushedStore);
                }


                if(highscore){
                    animateY=animateY+30;
                    drawBox(0,0,widthGlobal,animateY,0xF800);

                    char textScore[40];
                    sprintf(textScore, "%d", score); 
                    char textScoreName[40] ="Your score: ";
                    drawText(3, 3, strcat(textScoreName, textScore)); 
                    
                    if(animateY>=heightGlobal){
                        drawHighscorePage();
                        firstPress = true;
                        if(score>highscores[0]){
                            highscores[4] = highscores[3];
                            highscores[3] = highscores[2];
                            highscores[2] = highscores[1];
                            highscores[1] = highscores[0];
                            highscores[0] = score;
                        } else if (score>highscores[1]){
                            highscores[4] = highscores[3];
                            highscores[3] = highscores[2];
                            highscores[2] = highscores[1];
                            highscores[1] = score;
                        } else if (score>highscores[2]){
                            highscores[4] = highscores[3];
                            highscores[3] = highscores[2];
                            highscores[2] = score;
                        } else if (score>highscores[3]){
                            highscores[4] = highscores[3];
                            highscores[3] = score;
                        } else if (score>highscores[4]){
                            highscores[4] = score;
                        }
                        int offsetX = -15;
                        int offsetY = -106;
                        int scoreTemp;
                        for(int i=0; i < 5; i++){
                            scoreTemp = highscores[i];
                            while(scoreTemp){
                                drawSegNum(scoreTemp%10,offsetX,offsetY,0x0000);
                                offsetX -= 13;
                                scoreTemp = scoreTemp/10;
                            }
                            offsetX = -15;
                            offsetY += 26;
                        }
                        keyPushed = 0;
                        gameOverWait = true;
                    }
                }
                

                if(gameEnd && !highscore){
                    animateX=animateX+30;
                    drawBox(xBox[0]-animateX/2, yBox[0],animateX,60,0xF800);
                    drawBox(10,10,39,6,0xF800);
                    if(animateX>=widthGlobal){
                        drawEndPage();
                        
                        int offset = 0;
                        if (score==0){
                            drawSegNum(0,offset,0,0x0000);
                        } else {
                            int scoreTemp = score;
                            while(scoreTemp){
                                drawSegNum(scoreTemp%10,offset,0,0x0000);
                                offset -= 13;
                                scoreTemp = scoreTemp/10;
                            }
                        }
                        highscore = true;
                        firstPress = true;
                        clearOnce = 0;
                        timer = 0;
                        keyPushed = 0;
                    }
                    
                }
                

                waitForVsync(); // swap front and back buffers on VGA vertical sync
                pixelBufferStart = *(pixelCtrlPtr + 1); // new back buffer

                for (int j = 0; j < N; j++) {
                    drawTileClear(xBox[j], yBox[j], DYBox);
                    yBox[j] += DYBox;
                }
            }
            *(a9TimerPtr + 3) = fBit;
            *HEXptr = seg7[timer % 10] | seg7[timer/ 10] << 8;
            timer --;
        }
        
    }
}

//Draw the seg 7 display for the end screen
void drawSegNum(int number, int deltaX, int deltaY, short int color){
    //Top segment - drawBox(185+deltaX,172,11,3,color);
    //RightTop segment - drawBox(193+deltaX,172,3,8,color);
    //RightBottom segment - drawBox(193+deltaX,180,3,8,color);
    //Bottom segment - drawBox(185+deltaX,185,11,3,color);
    //LeftBottom segment - drawBox(185+deltaX,180,3,8,color);
    //LeftTop segment - drawBox(185+deltaX,172,3,8,color);
    //Middle segment - drawBox(185+deltaX,179,11,2,color);
    deltaX = deltaX + 26;
    switch(number) {
        case 9:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            return;
        case 8:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            drawBox(185+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            return;
        case 7:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            return;
        case 6:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(185+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,180+deltaY,3,8,color);
            return;
        case 5:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(185+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            return;
        case 4:
            drawBox(185+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            return;
        case 3:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            return;
        case 2:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(185+deltaX,179+deltaY,11,2,color);
            drawBox(185+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            return;
        case 1:
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            break;
        case 0:
            drawBox(185+deltaX,172+deltaY,11,3,color);
            drawBox(193+deltaX,172+deltaY,3,8,color);
            drawBox(193+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,185+deltaY,11,3,color);
            drawBox(185+deltaX,180+deltaY,3,8,color);
            drawBox(185+deltaX,172+deltaY,3,8,color);
            break;
    }
}

void drawStatus (int x0, int y0, bool correct, int keyPushedStore) {
    int xSize = 40;
    int ySize = 60;
    if(y0>=180){
        ySize = 240-y0;
    }

    for (int x = x0; x <= xSize+x0 ; x++) {
        for (int y = y0; y <= ySize+y0; y++) {
            if(!correct||keyPushedStore!=1) {
                break;
            }
            else {
                plotPixel(x, y, 0x07E0);
            }
        }
    }
}

//checks if the user input matches the tile drawn on screen, 1 correct, -1 incorrect, 0 no input
int checkTile (int keyPushed, int frontTile, int gamemode, int frontTilesSurprise) {
    // KEY[3] pushed = 8, KEY[2] = 4, KEY[1] = 2, KEY[0] = 1
    if(gamemode == 2 && frontTilesSurprise==1){
        return 1;
    } else if (gamemode == 2 && frontTilesSurprise==2){
        return 1;
    } else if (gamemode == 2  && frontTile == 0 && keyPushed == 0x802d && frontTilesSurprise==3){
        return 1;
    } else if (gamemode == 2  && frontTile == 1 && keyPushed == 0x8024 && frontTilesSurprise==3){
        return 1;
    } else if (gamemode == 2  && frontTile == 2 && keyPushed == 0x801d && frontTilesSurprise==3){
        return 1;
    } else if (gamemode == 2  && frontTile == 3 && keyPushed == 0x8015 && frontTilesSurprise==3){
        return 1;
    } else if (gamemode == 2 && frontTile == 0 && keyPushed == 0x8015  && frontTilesSurprise==4) {
        return 2;
    } else if (gamemode == 2 && frontTile == 1 && keyPushed == 0x801d && frontTilesSurprise==4) {
        return 2;
    } else if (gamemode == 2 && frontTile == 2 && keyPushed == 0x8024 && frontTilesSurprise==4) {
        return 2;
    } else if (gamemode == 2 && frontTile == 3 && keyPushed == 0x802d && frontTilesSurprise==4) {
        return 2;
    } else if (gamemode == 2 && frontTile == 0 && keyPushed == 0x8015  && frontTilesSurprise==0) {
        return 1;
    } else if (gamemode == 2 && frontTile == 1 && keyPushed == 0x801d && frontTilesSurprise==0) {
        return 1;
    } else if (gamemode == 2 && frontTile == 2 && keyPushed == 0x8024 && frontTilesSurprise==0) {
        return 1;
    } else if (gamemode == 2 && frontTile == 3 && keyPushed == 0x802d && frontTilesSurprise==0) {
        return 1;
    } else if (gamemode == 2 && (keyPushed==0x8015||keyPushed==0x801d||keyPushed==0x8024||keyPushed==0x802d) && (frontTilesSurprise==0||frontTilesSurprise==3||frontTilesSurprise==4)){
        return -1;
    } else if (gamemode == 1 && frontTile == 0 && keyPushed == 0x8015) {
        return 1;
    } else if (gamemode == 1 && frontTile == 1 && keyPushed == 0x801d) {
        return 1;
    } else if (gamemode == 1 && frontTile == 2 && keyPushed == 0x8024) {
        return 1;
    } else if (gamemode == 1 && frontTile == 3 && keyPushed == 0x802d) {
        return 1;
    } else if (gamemode == 1 && (keyPushed==0x8015||keyPushed==0x801d||keyPushed==0x8024||keyPushed==0x802d)){
        return -1;
    } else {
        return 0;
    }
}

//draws the piano tile where specified in black, give start coordinates of top left
//and ensure the size doesn't go off screen on the bottom
void drawTile(int x0, int y0, short int color) {
    int xSize = 40;
    int ySize = 60;
    if(y0>=180){
        ySize = heightGlobal-y0;
    }

    for (int x = x0; x <= xSize+x0 ; x++) {
        for (int y = y0; y <= ySize+y0; y++) {
            plotPixel(x, y, color);
        }
    }
}

//clear the drawn tiles
void drawTileClear(int x0, int y0, int dy) {
    int xSize = 40;
    int ySize = 60;
    int yBuffer = 20;
    if(y0>=180){
        ySize = heightGlobal-y0;
    }

    for (int x = x0; x <= xSize+x0 ; x++) {
        for (int y = y0-ySize-yBuffer; y <= ySize+y0+yBuffer; y++) {
            plotPixel(x, y, 0xFFFF);
        }
    }
}

void drawBox(int x0, int y0, int xSize, int ySize, short int color) {
    for (int x = x0; x <= x0+xSize; x++) {
        for (int y = y0; y <= y0+ySize; y++) {
            plotPixel(x, y, color);
        }
    }
}

//draws the pixel at a specified coordinate on the VGA display
void plotPixel(int x, int y, short int lineColor) {
    if(x<widthGlobal&&y<heightGlobal&&x>=0&&y>=0)
        *(short int *)(pixelBufferStart + (y << 10) + (x << 1)) = lineColor;
}

//clears the screen by drawing a pixel at every spot in white
void clearScreen() {
    for(int x=0; x < widthGlobal; x++) {
        for(int y=0; y < heightGlobal; y++) {
            plotPixel(x, y, 0xFFFF);
        }
    }
}

//Draws the title array image
void drawTitlePage() {
   int i = 0, j = 0;
    for (int k = 0; k < widthGlobal*heightGlobal*2-1; k+=2) {
        int red = ((titlePage[k+1] & 0xF8) >> 3) << 11;
        int green = (((titlePage[k] & 0xE0) >> 5)) | ((titlePage[k+1] & 0x7) << 3);
        int blue = (titlePage[k] & 0x1f);
        short int p = red | ((green << 5) | blue);
        plotPixel(0 + i, j, p);
        i += 1;
        if (i == widthGlobal) {
            i = 0;
            j += 1;
        }
    }
}

//Draws the end screen array image
void drawEndPage() {
   int i = 0, j = 0;
    for (int k = 0; k < widthGlobal*heightGlobal*2-1; k+=2) {
        int red = ((endPage[k+1] & 0xF8) >> 3) << 11;
        int green = (((endPage[k] & 0xE0) >> 5)) | ((endPage[k+1] & 0x7) << 3);
        int blue = (endPage[k] & 0x1f);
        short int p = red | ((green << 5) | blue);
        plotPixel(0 + i, j, p);
        i += 1;
        if (i == widthGlobal) {
            i = 0;
            j += 1;
        }
    }
}

//Draws the end screen array image
void drawHighscorePage() {
   int i = 0, j = 0;
    for (int k = 0; k < widthGlobal*heightGlobal*2-1; k+=2) {
        int red = ((highscorePage[k+1] & 0xF8) >> 3) << 11;
        int green = (((highscorePage[k] & 0xE0) >> 5)) | ((highscorePage[k+1] & 0x7) << 3);
        int blue = (highscorePage[k] & 0x1f);
        short int p = red | ((green << 5) | blue);
        plotPixel(0 + i, j, p);
        i += 1;
        if (i == widthGlobal) {
            i = 0;
            j += 1;
        }
    }
}

//waits for the timer to sest the F bit to 1, indicating that a second has passed
int timerCheck() {
    int fBit = 0;
    int keyPushed = 0;
    while (fBit == 0) {
        fBit = *(a9TimerPtr + 3);
        keyPushed = *KeyboardPointer;
        if (keyPushed == 0x8015 || keyPushed == 0x801d || keyPushed == 0x8024 || keyPushed == 0x802d) {
            return 1;
        }
    }
    *(a9TimerPtr + 3) = 1;
    return 0;
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

    if (y0 < y1) 
        yStep = 1;
    else
        yStep = -1;

    for(int x = x0; x<= x1; x++) {
        if (steep)
            plotPixel(y,x, colour);
        else 
            plotPixel(x, y, colour);

        error += deltay;

        if (error >= 0) {
            y += yStep;
            error -= deltax;
        }
    }
}

void drawText(int x, int y, char * textPtr) { 
    int offset;
    offset = (y << 7) + x;
    volatile char * characterBuffer = (char *)0xC9000000; 
    while (*(textPtr)) { 
        *(characterBuffer+offset) = *(textPtr); 
        textPtr++; 
        offset++;
    }
}

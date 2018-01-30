// -*- mode: c++; c-basic-offset: 2; indent-tabs-mode: nil; -*-
// Small example how to use the library.
// For more examples, look at demo-main.cc
//
// This code is public domain
// (but note, that the led-matrix library this depends on is GPL v2)

#include "led-matrix.h"
#include "graphics.h"

#include <getopt.h>
#include <string.h>

#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <termios.h>
#include "ncurses.h"

#define ROWS 32 
#define COLS 16
#define TRUE 1
#define FALSE 0


char Table[ROWS][COLS][4] = {0};
int score = 0;
char GameOn = TRUE;
double timer = 500000;

typedef struct {
    char **array;
    int width, row, col;
    int colour;
    int r,g,b;
} Shape;
Shape current;

const Shape ShapesArray[7]= {
    {(char *[]){(char []){0,1,1},(char []){1,1,0}, (char []){0,0,0}}, 3},                           //S_shape     
    {(char *[]){(char []){1,1,0},(char []){0,1,1}, (char []){0,0,0}}, 3},                           //Z_shape     
    {(char *[]){(char []){0,1,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //T_shape     
    {(char *[]){(char []){0,0,1},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //L_shape     
    {(char *[]){(char []){1,0,0},(char []){1,1,1}, (char []){0,0,0}}, 3},                           //ML_shape    
    {(char *[]){(char []){1,1},(char []){1,1}}, 2},                                                   //SQ_shape
    {(char *[]){(char []){0,0,0,0}, (char []){1,1,1,1}, (char []){0,0,0,0}, (char []){0,0,0,0}}, 4} //R_shape
};

const int digit[10][5][3] = {
	{{1, 1, 1}, {1, 0, 1}, {1, 0, 1}, {1, 0, 1}, {1, 1, 1}},
	{{1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}},
	{{1, 1, 1}, {1, 0, 0}, {1, 1, 1}, {0, 0, 1}, {1, 1, 1}},
	{{1, 1, 1}, {1, 0, 0}, {1, 1, 1}, {1, 0, 0}, {1, 1, 1}},
	{{1, 0, 1}, {1, 0, 1}, {1, 1, 1}, {1, 0, 0}, {1, 0, 0}},
	{{1, 1, 1}, {0, 0, 1}, {1, 1, 1}, {1, 0, 0}, {1, 1, 1}},
	{{1, 1, 1}, {0, 0, 1}, {1, 1, 1}, {1, 0, 1}, {1, 1, 1}},
	{{1, 1, 1}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}, {1, 0, 0}},
	{{1, 1, 1}, {1, 0, 1}, {1, 1, 1}, {1, 0, 1}, {1, 1, 1}},
	{{1, 1, 1}, {1, 0, 1}, {1, 1, 1}, {1, 0, 0}, {1, 1, 1}},
};

using rgb_matrix::GPIO;
using rgb_matrix::RGBMatrix;
using rgb_matrix::Canvas;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
	interrupt_received = true;
}


Shape CopyShape(Shape shape){
    Shape new_shape = shape;
    char **copyshape = shape.array;
    new_shape.array = (char**)malloc(new_shape.width*sizeof(char*));
    int i, j;
    for(i = 0; i < new_shape.width; i++){
        new_shape.array[i] = (char*)malloc(new_shape.width*sizeof(char));
        for(j=0; j < new_shape.width; j++) {
            new_shape.array[i][j] = copyshape[i][j];
        }
    }
    return new_shape;
}

void DeleteShape(Shape shape){
    int i;
    for(i = 0; i < shape.width; i++){
        free(shape.array[i]);
    }
    free(shape.array);
}

int CheckPosition(Shape shape){ //Check the position of the copied shape
    char **array = shape.array;
    int i, j;
    for(i = 0; i < shape.width;i++) {
        for(j = 0; j < shape.width ;j++){
            if((shape.col+j < 0 || shape.col+j >= COLS || shape.row+i >= ROWS)){ //Out of borders
                if(array[i][j]) //but is it just a phantom?
                    return FALSE;
            }
            else if(Table[shape.row+i][shape.col+j][0] && array[i][j])
                return FALSE;
        }
    }
    return TRUE;
}

void GetNewShape(){ //returns random shape
    Shape new_shape = CopyShape(ShapesArray[rand()%7]);
    new_shape.colour = rand()%3;
    new_shape.r = rand()%138 + 10;
    new_shape.b = rand()%138 + 10;
    new_shape.g = rand()%138 + 10;


    new_shape.col = rand()%(COLS-new_shape.width+1);
    new_shape.row = 0;
    DeleteShape(current);
    current = new_shape;
    if(!CheckPosition(current)){
        GameOn = FALSE;
    }
}

void RotateShape(Shape shape){ //rotates clockwise
    Shape temp = CopyShape(shape);
    int i, j, k, width;
    width = shape.width;
    for(i = 0; i < width ; i++){
        for(j = 0, k = width-1; j < width ; j++, k--){
                shape.array[i][j] = temp.array[k][i];
        }
    }
    DeleteShape(temp);
}

void WriteToTable(){
    int i, j;
    for(i = 0; i < current.width ;i++){
        for(j = 0; j < current.width ; j++){
            if(current.array[i][j]) {
                Table[current.row+i][current.col+j][0] = 1;
		Table[current.row+i][current.col+j][1] = current.r;
		Table[current.row+i][current.col+j][2] = current.g;
		Table[current.row+i][current.col+j][3] = current.b;
	    }
        }
    }
}

void Halleluyah_Baby(Canvas *canvas){ //checks lines
    int i, j, sum, count=0;
    for(i=0;i<ROWS;i++){
        sum = 0;
        for(j=0;j< COLS;j++) {
            sum+=Table[i][j][0]!=0?1:0;
        }
        if(sum==COLS){
            count++;
            int l, k;
            for(k = i;k >=1;k--)
                for(l=0;l<COLS;l++)
                    Table[k][l][0]=Table[k-1][l][0];
            for(l=0;l<COLS;l++)
                Table[k][l][0]=0;
        }
    }
    timer -= 1000; score += count;
    // this is for scoreboard

    int dy = 55;
    // digit[d][row][col]
    int hundreds = score%10;
    int thousands = int(score/10)%10;

    std::cout << "Hundreds: " << hundreds;
    std::cout << "\nThousands: " << thousands;

    for (int row = 0; row < 5; row++) {
 	for (int col = 0; col < 3; col++) {
	    canvas->SetPixel(row+dy, col + 4, 0, 0, 0);
	    canvas->SetPixel(row+dy, col, 0, 0, 0);
	    if (digit[thousands][row][col]) {
		canvas->SetPixel(row+dy, col + 4, 100, 100, 100);
	    }
	    if (digit[hundreds][row][col]) {
	        canvas->SetPixel(row + dy, col, 100, 100, 100);
	    }
	}
    }
}

void PrintTable(Canvas *canvas){
    char Buffer[ROWS][COLS] = {0};
    int i, j;
    for(i = 0; i < current.width ;i++){
        for(j = 0; j < current.width ; j++){
            if(current.array[i][j])
                Buffer[current.row+i][current.col+j] = current.array[i][j];
        }
    }
    clear();
    for(i = 0; i < ROWS ;i++){ 
        for(j = 0; j < COLS ; j++){
	    if (Buffer[i][j] > 0) {
		canvas->SetPixel(i, j, current.r, current.g, current.b);
	    } else {
		canvas->SetPixel(i, j, 0, 0, 0);
	    }
	    if (Table[i][j][0] > 0) {
		canvas->SetPixel(i, j, Table[i][j][1], Table[i][j][2], Table[i][j][3]);
	    }
            // printw("%c ", (Table[i][j][0] + Buffer[i][j])? 'O': '.'); // change this line
        }
        // printw("\n");
    }
    // printw("\nScore: %d\n", score);
    
}

void ManipulateCurrent(int action, Canvas *canvas){
    Shape temp = CopyShape(current);
    switch(action){
        case 's':
            temp.row++;  //move down
            if(CheckPosition(temp))
                current.row++;
            else {
                WriteToTable();
                Halleluyah_Baby(canvas); //check full lines, after putting it down
                GetNewShape();
            }
            break;
        case 'a':
            temp.col++;  //move right
            if(CheckPosition(temp))
                current.col++;
            break;
        case 'd':
            temp.col--;  //move left
            if(CheckPosition(temp))
                current.col--;
            break;
        case 'w':
            RotateShape(temp);  //yes
            if(CheckPosition(temp))
                RotateShape(current);
            break;
    }
    DeleteShape(temp);
    PrintTable(canvas);
}


int main(int argc, char *argv[]) {
	RGBMatrix::Options defaults;
	defaults.hardware_mapping = "regular";  // or e.g. "adafruit-hat"
	defaults.rows = 32;
	defaults.chain_length = 1;
	defaults.parallel = 1;
	defaults.show_refresh_rate = true;
	Canvas *canvas = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults);
	if (canvas == NULL)
		return 1;

	// It is always good to set up a signal handler to cleanly exit when we
	// receive a CTRL-C for instance. The DrawOnCanvas() routine is looking
	// for that.
	signal(SIGTERM, InterruptHandler);
	signal(SIGINT, InterruptHandler);

	srand(time(0));
    	score = 0;
	std::string scoreStr, timerStr;
    	int c;
    	struct timeval before, after;
    	gettimeofday(&before, NULL);

    	GetNewShape();
    	PrintTable(canvas);
    	while(GameOn){
		initscr();
		if (interrupt_received) 
			return 0;
		
		timeout(1000);
		if ((c = getch()) != ERR) {
			endwin();
          		ManipulateCurrent(c, canvas);
		}
		endwin();
        	gettimeofday(&after, NULL);
        	if (((double)after.tv_sec*1000000 + (double)after.tv_usec)-((double)before.tv_sec*1000000 + (double)before.tv_usec) > timer){ //time difference in microsec accuracy
            		before = after;
            		ManipulateCurrent('s', canvas);
        	}
    	}
    	DeleteShape(current);
    	return 0;
	

	// Animation finished. Shut down the RGB matrix.
	canvas->Clear();
	delete canvas;

	return 0;
}

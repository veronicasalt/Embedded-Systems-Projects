/*
    Adapted from Ivor Horton, "Beginning C", 3rd ed., apress, 2004 (ISBN: 1-59059-253-0).
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "library.h"
#include "graphics.h"
#include "touch.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

// Functions to be implemented in assembly

int __attribute__((weak)) Between(int min, int value, int max)
    {
    return (unsigned) (value - min) <= (unsigned) (max - min) ;
    }

int __attribute__((weak)) Count(int cells[], int numb, int value)
    {
    int count ;

    count = 0 ;
    while (numb-- != 0)
        {
        if (cells[numb] == value) count++ ;
        }

    return count ;
    }

#pragma GCC pop_options

#define HDR_ROWS    48
#define FTR_ROWS    16

#define FONT_SCORE  Font16
#define FONT_PROMPT Font12

#define SCORE_YPOS  (HDR_ROWS + 6)
#define PROMPT_YPOS (YPIXELS - FTR_ROWS - FONT_PROMPT.Height - 2)

#define CELL_SIZE   25
#define LINE_WIDTH  2

#define BOARD_ROWS  8
#define BOARD_COLS  8
#define BOARD_XPOS  ((XPIXELS - BOARD_COLS*(CELL_SIZE + LINE_WIDTH) - LINE_WIDTH)/2)
#define BOARD_YPOS  (SCORE_YPOS + FONT_SCORE.Height)
#define BOARD_COLOR COLOR_LIGHTGRAY

#define ENTRIES(a)  (sizeof(a)/sizeof(a[0]))

#define TS_XFUDGE   -4
#define TS_YFUDGE   -4

#define CPU_CLOCK_SPEED_MHZ 168

typedef enum {FALSE = 0, TRUE = 1} BOOL ;

static int colors[] = {BOARD_COLOR, COLOR_WHITE, COLOR_RED, COLOR_GREEN} ;
#define EMPTY       0
#define VALID       1
#define COMPUTER    2
#define HUMAN       3

typedef struct
    {
    int             rows ;
    int             cols ;
    int             xpos ;
    int             ypos ;
    int             cell_size ;
    int             line_width ;
    int             cells[0] ;
    } BOARD ;

typedef struct
    {
    const uint8_t * table ;
    const uint16_t  Width ;
    const uint16_t  Height ;
    } sFONT;

extern sFONT        Font8 ;
extern sFONT        Font12 ;    // Smaller font used in footer
extern sFONT        Font16 ;    // Larger font used in header
extern sFONT        Font20 ;
extern sFONT        Font24 ;

// Functions private to the main program
static int          BestHumanMove(BOARD *board) ;
static int          Cells(BOARD *board) ;
static void         ComputerMove(BOARD *board) ;
static BOARD *      CreateBoard(int rows, int cols, int xpos, int ypos, int cell_size, int line_width) ;
static void         DisplayBoard(BOARD *board) ;
static void         DisplayPrompt(char *text) ;
static void         Display1Score(BOARD *board, char *label, int who, int xpos, int mpier) ;
static void         DisplayScores(BOARD *board) ;
static void         DrawGrid(BOARD *board) ;
static void         DrawPiece(BOARD *board, int row, int col, int who) ;
static BOOL         FindMoves(BOARD *board, int player) ;
static void         HumanMove(BOARD *board) ;
static void         InitializeTouchScreen(void) ;
static void         MakeMove(BOARD *board, int cell, int player) ;
static void         SetFontSize(sFONT *font) ;

int main()
    {
    BOARD *board ;
    int player ;

    InitializeHardware(HEADER, "Lab 6E: Reversi") ;
    InitializeTouchScreen() ;
    board = CreateBoard(BOARD_ROWS, BOARD_COLS, BOARD_XPOS, BOARD_YPOS, CELL_SIZE, LINE_WIDTH) ;

    player = HUMAN ;
    for (;;)
        {
        DisplayScores(board) ;

        if (player == HUMAN)
            {
            if (FindMoves(board, HUMAN))
                {
                DisplayBoard(board) ;
                DisplayPrompt("Touch any white spot") ;
                HumanMove(board) ;
                }
            else break ;
            }

       else if (player == COMPUTER)
            {
            if (FindMoves(board, COMPUTER))
                {
                DisplayBoard(board) ;
                DisplayPrompt("Press blue pushbutton") ;
                WaitForPushButton() ;
                ComputerMove(board) ;
                }
            else break ;
            }

        player = (player == HUMAN) ? COMPUTER : HUMAN ;
        }

    DisplayBoard(board) ;
    DisplayPrompt("*** GAME OVER ***") ;

    return 0 ;
    }

BOARD *CreateBoard(int rows, int cols, int xpos, int ypos, int cell_size, int line_width)
    {
    int row, col ;

    BOARD *board = (BOARD *) malloc(sizeof(BOARD) + rows*cols*sizeof(int)) ;
    
    board->rows = rows ;
    board->cols = cols ;
    board->xpos = xpos ;
    board->ypos = ypos ;
    board->cell_size = cell_size ;
    board->line_width = line_width ;
    memset(board->cells, EMPTY, rows*cols*sizeof(int)) ;

    row = rows/2 ;  col = cols/2 ;
    board->cells[cols*(row-1) + col - 1] = COMPUTER ;
    board->cells[cols*(row  ) + col    ] = COMPUTER ;
    board->cells[cols*(row-1) + col    ] = HUMAN ;
    board->cells[cols*(row  ) + col - 1] = HUMAN ;

    DrawGrid(board) ;
    return board ;
    }

static void DrawGrid(BOARD *board)
    {
    int row, col, xpos, ypos, width, height, pixels ;

    pixels = board->cell_size + board->line_width ;

    SetColor(BOARD_COLOR) ;
    width  = board->cols*pixels ;
    height = board->rows*pixels ;
    FillRect(board->xpos, board->ypos, width, height) ;

    SetColor(COLOR_BLACK) ;

    xpos = board->xpos ;
    ypos = board->ypos ;
    width += board->line_width ;
    for (row = 0; row <= board->rows; row++, ypos += pixels)
        {
        FillRect(xpos, ypos, width, board->line_width) ;
        }

    xpos = board->xpos ;
    ypos = board->ypos ;
    height += board->line_width ;
    for (col = 0; col <= board->cols; col++, xpos += pixels)
        {
        FillRect(xpos, ypos, board->line_width, height) ;
        }
    }

static void DisplayPrompt(char *text)
    {
    int xpos = (XPIXELS - FONT_PROMPT.Width*strlen(text)) / 2 ;

    SetForeground(COLOR_WHITE) ;
    FillRect(0, PROMPT_YPOS, XPIXELS, FONT_PROMPT.Height) ;

    SetFontSize(&FONT_PROMPT) ;
    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_WHITE) ;
    DisplayStringAt(xpos, PROMPT_YPOS, text) ;
    }

static void DisplayScores(BOARD *board)
    {
    SetForeground(COLOR_WHITE) ;
    FillRect(0, SCORE_YPOS, XPIXELS, FONT_SCORE.Height) ;
    Display1Score(board, "You",      HUMAN,     board->xpos,            0) ;
    Display1Score(board, "Computer", COMPUTER,  XPIXELS - board->xpos, -1) ;
    }

static void Display1Score(BOARD *board, char *label, int who, int xpos, int mpier)
    {
    int radius, center_x, center_y ;
    char text[100] ;

    sprintf(text, "%s :%u", label, Count(board->cells, Cells(board), who)) ;
    xpos += FONT_SCORE.Width * mpier * strlen(text) ;
    
    radius = FONT_SCORE.Height/2 - 2 ;
    center_x = xpos + strlen(label)*FONT_SCORE.Width + 6 ;
    center_y = SCORE_YPOS + radius ;

    SetFontSize(&FONT_SCORE) ;
    SetBackground(COLOR_WHITE) ;
    SetForeground(COLOR_BLACK) ;
    DisplayStringAt(xpos, SCORE_YPOS, text) ;

    SetForeground(colors[who]) ;
    FillCircle(center_x, center_y, radius) ;
    SetForeground(COLOR_BLACK) ;
    DrawCircle(center_x, center_y, radius) ;
    }

static void ComputerMove(BOARD *board)
    {
    int cell, cells, best, worst, bytes ;
    BOARD *temp ;

    cells = Cells(board) ;
    bytes = sizeof(BOARD) + cells * sizeof(int) ;
    temp = (BOARD *) malloc(bytes) ;

    best = 0 ;
    worst = cells ;
    for (cell = 0; cell < cells; cell++)
        {
        int score ;

        if (board->cells[cell] != VALID) continue ;
 
        memcpy(temp, board, bytes) ;
        temp->cells[cell] = COMPUTER ;
        FindMoves(temp, HUMAN) ;
        score = BestHumanMove(temp) ;
        if (score < worst)
            {
            worst = score ;
            best = cell ;
            }
        }
    free(temp) ;

    MakeMove(board, best, COMPUTER) ;
    }

static int BestHumanMove(BOARD *board)
    {
    int cell, cells, best, bytes ;
    BOARD *temp ;

    cells = Cells(board) ;
    bytes = sizeof(BOARD) + cells * sizeof(int) ;
    temp = (BOARD *) malloc(bytes) ;

    best = 0 ;
    for (cell = 0; cell < cells; cell++)
        {
        int score ;

        if (board->cells[cell] != VALID) continue ;

        memcpy(temp, board, bytes) ;
        temp->cells[cell] = HUMAN ;
        score = Count(board->cells, Cells(board), HUMAN) - Count(board->cells, Cells(board), COMPUTER) ;
        if (score > best) best = score ;
        }
    free(temp) ;

    return best ;
    }

static BOOL FindMoves(BOARD *board, int player)
    {
    int opponent, *pcell ;
    int cell, cells ;
    BOOL hasMoves ;

    hasMoves = FALSE ;
    cells = Cells(board) ;

    // Reset any left-over "VALID" cells to "EMPTY"
    pcell = board->cells ;
    for (cell = 0; cell < cells; cell++, pcell++)
        {
        if (*pcell == VALID) *pcell = EMPTY ;
        }

    // Mark all valid moves
    opponent = (player == HUMAN) ? COMPUTER : HUMAN ;
    pcell = board->cells ;
    for (cell = 0; cell < cells; cell++, pcell++)
        {
        int row, col, drow ;

        if (*pcell != EMPTY) continue ;

        row = cell / board->cols ;
        col = cell % board->cols ;  

        for(drow = -1; drow <= 1; drow++)
            {
            int dcol, r, c ;

            for (dcol = -1; dcol <= 1; dcol++)
                {
                if (drow == 0 && dcol == 0) continue ;

                if (!Between(0, r = row + drow, board->rows - 1)) continue ;
                if (!Between(0, c = col + dcol, board->cols - 1)) continue ;

                if (board->cells[board->cols*r + c] != opponent) continue ;

                for(;;)
                    {
                    int cell ;

                    if (!Between(0, r += drow, board->rows - 1)) break ;
                    if (!Between(0, c += dcol, board->cols - 1)) break ;

                    cell = board->cells[board->cols*r + c] ;
                    if (cell == EMPTY) break ;
                    if (cell == player)
                        {
                        *pcell = VALID ;
                        hasMoves = TRUE ;
                        break ;
                        }
                    }
                }
            }
        }

    return hasMoves ; 
    }

static void InitializeTouchScreen(void)
    {
    static char *message[] =
        {
        "If this message remains on",
        "the screen, there is an",
        "initialization problem with",
        "the touch screen. This does",
        "NOT mean that there is a",
        "problem with your code.",
        " ",
        "To correct the problem,",
        "disconnect and reconnect",
        "the power.",
        NULL
        } ;
    char **pp ;
    int x, y ;

    x = 25 ;
    y = 100 ;
    for (pp = message; *pp != NULL; pp++)
        {
        DisplayStringAt(x, y, *pp) ;
        y += 12 ;
        }
    TS_Init() ;
    ClearDisplay() ;
    }

static void SetFontSize(sFONT *font)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    BSP_LCD_SetFont(font) ;
    }

static void DisplayBoard(BOARD *board)
    {
    int *pcell ;
    int cell ;

    pcell = board->cells ;
    for (cell = 0; cell < Cells(board); cell++, pcell++)
        {
        int row = cell / board->cols ;
        int col = cell % board->cols ;
        DrawPiece(board, row, col, *pcell) ;
        }
    }

static void HumanMove(BOARD *board)
    {
    int xlft, ytop, pixels ;

    xlft = board->xpos + board->line_width ;
    ytop = board->ypos + board->line_width ;
    pixels  = board->cell_size + board->line_width ;
    for (;;)
        {
        int x, y, cell ;
        int *pcell ;

        while (!TS_Touched()) ;
        x = TS_GetX() + TS_XFUDGE ;
        y = TS_GetY() + TS_YFUDGE ;

        pcell = board->cells ;
        for (cell = 0; cell < Cells(board); cell++, pcell++)
            {
            int min ;

            min = ytop + pixels*(cell / board->cols) ;
            if (Between(min, y, min + board->cell_size - 1) == FALSE) continue ;

            min = xlft + pixels*(cell % board->cols) ;
            if (Between(min, x, min + board->cell_size - 1) == FALSE) continue ;

            if (*pcell == VALID)
                {
                MakeMove(board, cell, HUMAN) ;
                return ;
                }
            }
        }
    }

static void DrawPiece(BOARD *board, int row, int col, int owner)
    {
    int center_x, center_y, width, offset, radius ;

    width  = board->cell_size + board->line_width ;
    offset = board->cell_size/2 + board->line_width ;
    radius = board->cell_size/2 - board->line_width ;
    center_x = board->xpos + col*width + offset ;
    center_y = board->ypos + row*width + offset ;
    SetForeground(colors[owner]) ;
    FillCircle(center_x, center_y, radius) ;
    if (owner == EMPTY) return ;
    SetForeground(COLOR_BLACK) ;
    DrawCircle(center_x, center_y, radius) ;
    }

static void MakeMove(BOARD *board, int cell, int player)
    {
    int opponent ;
    int row, drow ;

    opponent = (player == HUMAN) ? COMPUTER : HUMAN ;

    board->cells[cell] = player ;

    row = cell / board->cols - 1 ;
    for (drow = -1; drow <= 1; drow++, row++)
        {
        int col, dcol ;

        col = cell % board->cols - 1 ;
        for (dcol = -1; dcol <= 1; dcol++, col++)
            { 
            if (drow == 0 && dcol == 0) continue ;

            if (!Between(0, row, board->rows - 1)) continue ;
            if (!Between(0, col, board->cols - 1)) continue ;

            if (board->cells[board->cols*row + col] == opponent)
                {
                int r = row ;
                int c = col ;

                for (;;)
                    {
                    int *pcell ;

                    if (!Between(0, r += drow, board->cols - 1)) break ;
                    if (!Between(0, c += dcol, board->rows - 1)) break ;

                    pcell = &board->cells[board->cols*r + c] ;
                    if (*pcell == EMPTY) break ; 
                    if (*pcell == VALID) break ; 
                    if (*pcell == player)
                        {
                        for (;;)
                            {
                            pcell -= board->cols*drow + dcol ;
                            if (*pcell != opponent) break ;
                            *pcell = player ;
                            }
                        break ;
                        } 
                    }
                }
            }
        }
    }

static int Cells(BOARD *board)
    {
    return board->rows * board->cols ;
    }


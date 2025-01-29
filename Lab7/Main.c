/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

// Adapted from code posted at https://codereview.stackexchange.com/questions/136406/tetris-in-c-in-200-lines

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "library.h"
#include "graphics.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

typedef enum {FALSE = 0, TRUE = 1} BOOL ;

// Functions to be implemented in ARM assembly
BOOL __attribute__((weak)) GetBit(uint16_t *bits, uint32_t row, uint32_t col)
    {
#ifdef  BITWISE
    uint32_t shift = 4*row + col ;
    return (*bits >> shift) & 1 ;
#else   // BITBANDING
    uint32_t bitpos = 4*row + col ;
    uint32_t *bbadrs = (uint32_t *) (0x22000000 + (((uint32_t) bits - 0x20000000) << 5) + (bitpos << 2)) ;
    return *bbadrs ;
#endif
    }

void __attribute__((weak)) PutBit(BOOL value, uint16_t *bits, uint32_t row, uint32_t col)
    {
#ifdef  BITWISE
    uint32_t bitpos = 4*row + col ;
    uint16_t mask = 1 << bitpos ;
    if (value == TRUE)  *bits |= mask ;
    else                *bits &= ~mask ;
#else   // BITBANDING
    uint32_t bitpos = 4*row + col ;
    uint32_t *bbadrs = (uint32_t *) (0x22000000 + (((uint32_t) bits - 0x20000000) << 5) + (bitpos << 2)) ;
    *bbadrs = value ;
#endif
    }

#pragma GCC pop_options

#define ENTRIES(a)              (sizeof(a)/sizeof(a[0]))

#define CPU_SPEED_MHZ           168     // Also speed of DWT_CYCCNT used by GetClockCycleCount()

#define ROW_OFFSET              49      // y-pixel start position of usable display area
#define COL_OFFSET              0       // x-pixel start position of usable display area

// Game parameters
#define ROWS                    17      // Number of vertical cells in the playing field
#define COLS                    16      // Number of horizontal cells in the playing field

#define MSEC_PER_TILT           150     // How often the game checks for left or right tilt
#define MSEC_PER_DOWN           500     // How often the game moves a shape down one row

#define CYCLES_PER_TILT         (MSEC_PER_TILT*CPU_SPEED_MHZ*1000)
#define CYCLES_PER_DOWN         (MSEC_PER_DOWN*CPU_SPEED_MHZ*1000)

#define CELL_WIDTH              15      // horizontal pixels per cell
#define CELL_HEIGHT             15      // vertical pixels per cell

typedef struct
    {
    uint16_t                    array ;     // An array of four nibbles - one per row of the Shape
    uint32_t                    size ;      // shapes are square, so this is the number of rows and cols
    uint32_t                    color ;     // Color of the shape
    uint32_t                    row ;       // current row position of upper-left-hand corner of shape
    uint32_t                    col ;       // current col position of upper-left-hand corner of shape
    } SHAPE ;

typedef struct
    {
    const uint8_t *             table ;
    const uint16_t              Width ;
    const uint16_t              Height ;
    } sFONT;

#define COUNT(a)                (sizeof(a)/sizeof(a[0]))

#define SENSITIVITY_250DPS      8.75
#define SENSITIVITY_500DPS      17.50
#define SENSITIVITY_2000DPS     70.00

// Library function prototypes for Gyro sensor (not included in library.h)
extern void                     GYRO_IO_Init(void) ;
extern void                     GYRO_IO_Write(uint8_t* data, uint8_t port, uint16_t bytes) ;
extern void                     GYRO_IO_Read(uint8_t* data, uint8_t port, uint16_t bytes) ;
extern sFONT                    Font8, Font12, Font16, Font20, Font24 ;

const int                       GYRO_CTRL_REG1 = 0x20 ;
#define GYRO_DR1_FLAG           (1 << 7)
#define GYRO_DR0_FLAG           (1 << 6)
#define GYRO_BW1_FLAG           (1 << 5)
#define GYRO_BW0_FLAG           (1 << 4)
#define GYRO_PD_FLAG            (1 << 3)
#define GYRO_ZEN_FLAG           (1 << 2)
#define GYRO_YEN_FLAG           (1 << 1)
#define GYRO_XEN_FLAG           (1 << 0)

const int                       GYRO_CTRL_REG4 = 0x23 ;
#define GYRO_BDU_FLAG           (1 << 7)
#define GYRO_FS1_FLAG           (1 << 5)
#define GYRO_FS0_FLAG           (1 << 4)

const int                       GYRO_CTRL_REG5 = 0x24 ;
#define GYRO_OSEL1_FLAG         (1 << 1)
#define GYRO_HPEN_FLAG          (1 << 4)

const int                       GYRO_STAT_REG = 0x27 ;
#define GYRO_ZYXDA_FLAG         (1 << 3)

const int                       GYRO_DATA_REG = 0x28 ;

#define SHAPE_DOWN              0
#define SHAPE_ROTATE            1
#define SHAPE_RIGHT             2
#define SHAPE_LEFT              3
#define SHAPE_DROP              4

static uint32_t                 Table[ROWS][COLS] ;         // keeps track of cells in the playing field
static BOOL                     GameOn ;                    // Chenaged to FALSE when game is finished
static SHAPE                    current, temp ;
static unsigned                 score ;

static SHAPE ShapesArray[] =
    {
    {0x0063, 3},    // S shape
    {0x0036, 3},    // Z shape
    {0x0072, 3},    // T shape
    {0x0017, 3},    // L shape
    {0x0074, 3},    // J shape
    {0x0033, 2},    // O shape
    {0x00F0, 4}     // I shape
    } ;

static void                     CalibrateGyroscope(float Offsets[]) ;
static BOOL                     Conflict(SHAPE *shape) ;
static void                     GetNewShape(void) ;
static void                     IncreaseScore(unsigned points) ;
static void                     InitializeGyroscope(void) ;
static void                     Instructions(void) ;
static void                     LEDs(int grn_on, int red_on) ;
static void                     MoveThisShape(int action) ;
static void                     PaintCell(int row, int col, int color) ;
static void                     PaintShape(int color) ;
static void                     PlayGame(float bias[]) ;
static void                     ReadAngularRate(float pfData[]) ;
static void                     RotateShape(SHAPE *shape) ;
static int                      SanityChecksOK(void) ;
static void                     SetFontSize(sFONT *Font) ;
static void                     CollapseOneRow(void) ;
static void                     WriteToTable(void) ;

int main()
    {
    float bias[3] ;

    InitializeHardware(HEADER, "Lab 7E: Tetris & Gyros") ;
    if (!SanityChecksOK()) return 255 ;
    InitializeGyroscope() ;
    CalibrateGyroscope(bias) ;

    while (1)
        {
        ClearDisplay() ;
        PlayGame(bias) ;
        WaitForPushButton() ;
        }

    return 0 ;
    }

static void PlayGame(float bias[])
    {
    static float dps[3] = {0.0, 0.0, 0.0} ;
    uint32_t now, down_timeout, tilt_timeout ;
    const uint32_t samples_per_sec = 760 ;
    const float gyro_period = 1.0 / samples_per_sec ;
    float degr_roll ;
    int row, col ;
    uint8_t sts ;

    for (row = 0; row < ROWS; row++)
        {
        for (col = 0; col < COLS; col++)
            {
            Table[row][col] = COLOR_WHITE ;
            }
        }

    GetNewShape() ;
    PaintShape(current.color) ;
    degr_roll = 0.0 ;
    score = 0 ;
    IncreaseScore(0) ;

    now = GetClockCycleCount() ;
    down_timeout = now + CYCLES_PER_DOWN ;
    tilt_timeout = now + CYCLES_PER_TILT ;

    GameOn = TRUE ;
    while (GameOn)
        {
        // Check to see if new gyro data is available
        GYRO_IO_Read(&sts, GYRO_STAT_REG, sizeof(sts)) ;
        if ((sts & GYRO_ZYXDA_FLAG) != 0)
            {
            ReadAngularRate(dps) ;
            dps[0] -= bias[0] ;
            dps[1] -= bias[1] ;
            dps[2] -= bias[2] ;

            // Perform trapezoidal integration to get position
            degr_roll += gyro_period * dps[1] ;
            }

        if ((int) (tilt_timeout - GetClockCycleCount()) < 0)
            {
            if (degr_roll < -5.0)       MoveThisShape(SHAPE_LEFT) ;
            else if (degr_roll > +5.0)  MoveThisShape(SHAPE_RIGHT) ;
            if (dps[0] > 40.0)          MoveThisShape(SHAPE_DROP) ;
            tilt_timeout += CYCLES_PER_TILT ;
            }

        if ((int) (down_timeout - GetClockCycleCount()) < 0)
            {
            MoveThisShape(SHAPE_DOWN) ;
            down_timeout += CYCLES_PER_DOWN ;
            }

        if (PushButtonPressed())
            {
            WaitForPushButton() ;
            MoveThisShape(SHAPE_ROTATE) ;
            degr_roll = 0.0 ;
            }
        }
    }

static void IncreaseScore(unsigned points)
    {
    char footer[100] ;

    score += points ;
    sprintf(footer, "Lab 7E: Tetris & Gyros (%d pts)", score) ;
    DisplayFooter(footer) ;
    }

static BOOL Conflict(SHAPE *shape)
    {
    int row = shape->row ;
    for (int r = 0; r < shape->size; r++, row++)
        {
        int col = shape->col ;
        for (int c = 0; c < shape->size; c++, col++)
            {
            if (GetBit(&shape->array, r, c) == 0) continue ;

            // Is this cell of the table already occupied?
            if (Table[row][col] != COLOR_WHITE) return TRUE ;

            if (col < 0 || col >= COLS || row >= ROWS) return TRUE ;
            }
        }

    return FALSE ;
    }

static void GetNewShape(void)
    {
    static int colors[] = {COLOR_RED, COLOR_BLUE, COLOR_ORANGE, COLOR_YELLOW, COLOR_MAGENTA, COLOR_CYAN, COLOR_GREEN} ;
    static int prev_shape = 0 ;
    static int prev_color = 0 ;
    int shape, color ;

    do shape = GetRandomNumber() % ENTRIES(ShapesArray) ;
    while (shape == prev_shape) ;
    prev_shape = shape ;
    current = ShapesArray[shape] ;

    do color = GetRandomNumber() % ENTRIES(colors) ;
    while (color == prev_color) ;
    prev_color = color ;
    current.color = colors[color] ;

    current.col = GetRandomNumber() % (COLS - current.size + 1) ;
    current.row = 0 ;

    if (Conflict(&current)) GameOn = FALSE ;
    }

static void RotateShape(SHAPE *shape) //rotates clockwise
    {
    temp = *shape ;
    for (int r = 0; r < shape->size; r++)
        {
        int col = shape->size - 1 ;
        for (int c = 0; c < shape->size ; c++, col--)
            {
            BOOL value = GetBit(&temp.array, col, r) ;
            PutBit(value, &shape->array, r, c) ;
            }
        }
    }

static void WriteToTable(void)
    {
    int row = current.row ;
    for (int r = 0; r < current.size; r++, row++)
        {
        int col = current.col ;
        for (int c = 0; c < current.size; c++, col++)
            {
            if (GetBit(&current.array, r, c) != 0)
                {
                Table[row][col] = current.color ;
                }
            }
        }
    }

static void CollapseOneRow(void)
    {
    for (int row = 0; row < ROWS; row++)
        {
        int filled = 0 ;

        for (int col = 0; col < COLS; col++)
            {
            if (Table[row][col] != COLOR_WHITE) filled++ ;
            }

        // Do nothing if any col of this row is empty
        if (filled < COLS) continue ;

        // Remove full row and shift above rows down
        for (int r = row; r > 0; r--)
            {
            for (int col = 0; col < COLS; col++)
                {
                Table[r][col] = Table[r-1][col] ;
                PaintCell(r, col, Table[r-1][col]) ;
                }
            }

        // Clear the top row
        for (int col = 0; col < COLS; col++)
            {
            Table[0][col] = COLOR_WHITE ;
            PaintCell(0, col, COLOR_WHITE) ;
            }

        IncreaseScore(100) ;
        }
    }

static void PaintCell(int row, int col, int color)
    {
    unsigned xpos = COL_OFFSET + CELL_WIDTH*col ;
    unsigned ypos = ROW_OFFSET + CELL_HEIGHT*row ;

    SetColor(color) ;
    FillRect(xpos, ypos, CELL_WIDTH, CELL_HEIGHT) ;
    if (color != COLOR_WHITE)
        {
        SetColor(COLOR_BLACK) ;
        DrawRect(xpos, ypos, CELL_WIDTH-1, CELL_HEIGHT-1) ;
        }
    }

static void PaintShape(int color)
    {
    int row = current.row ;
    for (int r = 0; r < current.size; r++, row++)
        {
        int col = current.col ;
        for (int c = 0; c < current.size; c++, col++)
            {
            if (GetBit(&current.array, r, c) != 0) PaintCell(row, col, color) ;
            }
        }
    }

static void MoveThisShape(int action)
    {
    temp = current ;

    PaintShape(COLOR_WHITE) ;
    switch (action)
        {
        case SHAPE_DROP:
            do temp.row++ ; 
            while (!Conflict(&temp)) ;
            current.row = temp.row - 1 ;
            break ;

        case SHAPE_DOWN:
            temp.row++ ;
            if (!Conflict(&temp))
                {
                current.row++ ;
                break ;
                }

            PaintShape(current.color) ;
            WriteToTable() ;
            CollapseOneRow() ;
            GetNewShape() ;
            IncreaseScore(1) ;
            break ;

        case SHAPE_RIGHT:
            temp.col++ ;
            if (!Conflict(&temp)) current.col++ ;
            break ;

        case SHAPE_LEFT:
            temp.col-- ;
            if (!Conflict(&temp)) current.col-- ;
            break ;

        case SHAPE_ROTATE:
            RotateShape(&temp) ;
            if (!Conflict(&temp)) RotateShape(&current) ;
            break ;
        }

    if (GameOn) PaintShape(current.color) ;
    }

static void InitializeGyroscope(void)
    {
    uint8_t tmpreg ;

    GYRO_IO_Init() ;

    // Enable full scale = 250 dps
    tmpreg = 0x00 ;
    GYRO_IO_Write(&tmpreg, GYRO_CTRL_REG4, sizeof(tmpreg)) ;

    // Enable X, Y and Z channels; 760 Hz ODR; Normal Mode
    tmpreg = GYRO_DR1_FLAG|GYRO_DR0_FLAG|GYRO_PD_FLAG|GYRO_XEN_FLAG|GYRO_YEN_FLAG|GYRO_ZEN_FLAG ;
    GYRO_IO_Write(&tmpreg, GYRO_CTRL_REG1, sizeof(tmpreg)) ;

    tmpreg = 0x10 ;
    GYRO_IO_Write(&tmpreg, GYRO_CTRL_REG5, sizeof(tmpreg));
    }

static void ReadAngularRate(float pfData[])
    {
    const float sensitivity = SENSITIVITY_250DPS / 1000.0 ;
    int16_t tmpbuffer[3] ;
  
    GYRO_IO_Read((uint8_t *) tmpbuffer, GYRO_DATA_REG, 6) ;
    for (int i = 0; i < COUNT(tmpbuffer); i++)
        {
        *pfData++ = sensitivity * tmpbuffer[i]  ;
        }
    }

static int SanityChecksOK(void)
    {
    int row, col, shift, bugs, bit ;
    static int bits ;

    bugs = 0 ;
    printf("\n\n\n") ;

    bits = 0 ;
    row = GetRandomNumber() % 4 ;
    col = GetRandomNumber() % 4 ;
    PutBit(1, (uint16_t *) &bits, row, col) ;
    shift = 4*row + col ;
    if (bits != (1 << shift))
        {
        printf("bits = 0x0000;\nPutBit(1, &bits, row=%d, col=%d);\nbits --> %04X\n\n", row, col, bits & 0xFFFF) ;
        bugs++ ;
        }

    bits = 0xFFFFFFFF ;
    row = GetRandomNumber() % 4 ;
    col = GetRandomNumber() % 4 ;
    PutBit(0, (uint16_t *) &bits, row, col) ;
    shift = 4*row + col ;
    if (bits != ~(1 << shift))
        {
        printf("bits = 0xFFFF;\nPutBit(0, &bits, row=%d, col=%d);\nbits --> %04X\n\n", row, col, bits & 0xFFFF) ;
        bugs++ ;
        }

    row = GetRandomNumber() % 4 ;
    col = GetRandomNumber() % 4 ;
    shift = 4*row + col ;
    bits = 1 << shift ;
    bit = GetBit((uint16_t *) &bits, row, col) ;
    if (bit != 1)
        {
        printf("bits = %04X;\nGetBit(&bits, row=%d, col=%d) --> %d\n\n", bits & 0xFFFF, row, col, bit) ;
        bugs++ ;
        }

    row = GetRandomNumber() % 4 ;
    col = GetRandomNumber() % 4 ;
    shift = 4*row + col ;
    bits = ~(1 << shift) ;
    bit = GetBit((uint16_t *) &bits, row, col) ;
    if (bit != 0)
        {
        printf("bits = %04X;\nGetBit(&bits, row=%d, col=%d) --> %d\n\n", bits & 0xFFFF, row, col, bit) ;
        bugs++ ;
        }

    if (bugs != 0)
        {
        SetForeground(COLOR_WHITE) ;
        SetBackground(COLOR_RED) ;
        DisplayStringAt(0, ROW_OFFSET, "Sanity Check Errors:" ) ;
        }

    LEDs(!bugs, bugs) ;
    return bugs == 0 ;
    }

static void LEDs(int grn_on, int red_on)
    {
    static uint32_t * const pGPIOG_MODER    = (uint32_t *) 0x40021800 ;
    static uint32_t * const pGPIOG_ODR      = (uint32_t *) 0x40021814 ;
    
    *pGPIOG_MODER |= (1 << 28) | (1 << 26) ;    // output mode
    *pGPIOG_ODR &= ~(3 << 13) ;         // both off
    *pGPIOG_ODR |= (grn_on ? 1 : 0) << 13 ;
    *pGPIOG_ODR |= (red_on ? 1 : 0) << 14 ;
    }

static void SetFontSize(sFONT *Font)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    BSP_LCD_SetFont(Font) ;
    }

static void Instructions(void)
    {
    static char *msg[] =
        {
        "CALIBRATING GYRO",
        "----------------",
        "",
        "Place board on a",
        "level surface and",
        "press blue button",
        "",
        "Do not move board",
        "until game begins"
        } ;
    uint32_t y ;

    SetFontSize(&Font16) ;
    y = 100 ;
    for (int line = 0; line < COUNT(msg); line++)
        {
        DisplayStringAt(20, y, (uint8_t *) msg[line]) ;
        y += Font16.Height ;
        }
    }

static void CalibrateGyroscope(float bias[])
    {
    float X_Bias, Y_Bias, Z_Bias ;
    const uint32_t samples = 1000 ;
    uint8_t sts ;

    Instructions() ;
    WaitForPushButton() ;

    X_Bias = Y_Bias = Z_Bias = 0.0 ;
    for (int i = 0; i < samples; i++)
        {
        do GYRO_IO_Read(&sts, GYRO_STAT_REG, sizeof(sts)) ;
        while ((sts & GYRO_ZYXDA_FLAG) == 0) ;
        ReadAngularRate(bias) ;
        X_Bias += bias[0] ;
        Y_Bias += bias[1] ;
        Z_Bias += bias[2] ;
        }

    /* Get offset value on X, Y and Z */
    bias[0] = X_Bias / samples ;
    bias[1] = Y_Bias / samples ;
    bias[2] = Z_Bias / samples ;

    ClearDisplay() ;
    }

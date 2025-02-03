/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "library.h"
#include "graphics.h"

#pragma GCC push_options
#pragma GCC optimize ("O0")

// Functions to be implemented in assembly ...
void __attribute__((weak)) WritePixel(int x, int y, uint8_t colorIndex, uint8_t frameBuffer[256][240])
    {
    frameBuffer[y][x] = colorIndex ;
    }

uint8_t * __attribute__((weak)) BitmapAddress(char ascii, uint8_t *fontTable, int charHeight, int charWidth)
    {
    uint32_t bytesPerRow, bytesPerChar ;

    bytesPerRow = (charWidth + 7) / 8 ;
    bytesPerChar = charHeight * bytesPerRow ;
    return fontTable + (ascii - ' ') * bytesPerChar ;
    }

uint32_t __attribute__((weak)) GetBitmapRow(uint8_t *prow)
    {
    return (prow[0] << 24) | (prow[1] << 16) | (prow[2] << 8) | prow[3] ;
    }

#pragma GCC pop_options

typedef uint32_t            CLR_RGB32 ;
typedef uint8_t             CLR_INDEX ;
typedef enum {FALSE = 0, TRUE = 1} BOOL ;

typedef struct
    {
    uint32_t                CR ;        // Control register
    uint32_t                ISR ;       // Interrupt Status Register 
    uint32_t                IFCR ;      // Interrupt flag clear register
    uint32_t                FGMAR ;     // Foreground memory address register
    uint32_t                FGOR ;      // Foreground offset register 
    uint32_t                BGMAR ;     // Background memory address register
    uint32_t                BGOR ;      // Background offset register
    uint32_t                FGPFCCR ;   // Foreground PFC control register
    uint32_t                FGCOLR ;    // Foreground color register
    uint32_t                BGPFCCR ;   // Background PFC control register
    uint32_t                BGCOLR ;    // Background color register
    uint32_t                FGCMAR ;    // Foreground CLUT memory address register
    uint32_t                BGCMAR ;    // Background CLUT memory address register
    uint32_t                OPFCCR ;    // Output PFC control register
    uint32_t                OCOLR ;     // Output color register 
    uint32_t                OMAR ;      // Output memory address register
    uint32_t                OOR ;       // Output offset register
    uint32_t                NLR ;       // Number of line register
    uint32_t                LWR ;       // Line watermark register
    uint32_t                AMTCR ;     // AHB master timer configuration register
    } CHROM_ART ;

typedef struct
    {
    const uint8_t *         table ;
    const uint16_t          Width ;
    const uint16_t          Height ;
    } sFONT;

#define X_MIN               0
#define Y_MIN               48

#define WIDTH               XPIXELS
#define HEIGHT              (YPIXELS - Y_MIN - 16)

#define XSIZE               XPIXELS
#define YSIZE               (HEIGHT - 16)

#define ITEMS(a)            (sizeof(a)/sizeof(a[0]))
#define COLOR_INDEX(hue)    ((255*(hue))/360)
#define HUE_RED             0
#define HUE_YLW             60
#define HUE_GRN             120

#define DISPLAY_XOFF        0
#define DISPLAY_YOFF        Y_MIN

#define PI                  3.14159

#define CPU_CLOCK_SPEED_MHZ 168

typedef CLR_INDEX           FRAME[HEIGHT][WIDTH] ;

extern sFONT                Font8, Font12, Font16, Font20, Font24 ;

static BOOL                 Aborted(void) ;
static void                 BarnsleyFernFractal(void) ;
static void                 ChromArtInitialize(void) ;
static void                 ChromArtWaitForDMA(void) ;
static void                 ChromArtXferFrameBuffer(CLR_RGB32 *screen_pixels, FRAME frame_pixels) ;
static void                 FractalTitle(char *title) ;
static uint32_t             GetTimeout(uint32_t msec) ;
static CLR_RGB32            HSV2RGB(float hue, float sat, float val) ;
static void                 JuliaSetFractal(void) ;
static void                 LEDs(int grn_on, int red_on) ;
static void                 MandelbrotSetFractal(void) ;
static CLR_RGB32            PackRGB(int red, int grn, int blu) ;
static void                 PutChar(int x, int y, char c, sFONT *font) ;
static void                 PutString(int x, int y, char *str, sFONT *font) ;
static int                  SanityChecksOK(void) ;
static void                 TextColor(CLR_INDEX color) ;
static void                 WaitForTimeout(uint32_t timeout) ;

static CLR_INDEX            textColor ;
static uint32_t * const     AHB1ENR         = (uint32_t *)  0x40023800 ;
static CHROM_ART * const    DMA2D           = (CHROM_ART *) 0x4002B000 ;
static CLR_RGB32 * const    FG_CLUT         = (CLR_RGB32 *) 0x4002B400 ; 
static const CLR_INDEX      INDEX_RED       = COLOR_INDEX(HUE_RED) ;
static const CLR_INDEX      INDEX_YLW       = COLOR_INDEX(HUE_YLW) ;
static const CLR_INDEX      INDEX_GRN       = COLOR_INDEX(HUE_GRN) ;
static CLR_RGB32 * const    screen_pixels   = (CLR_RGB32 *) 0xD0000000 ;
static FRAME                frame_pixels ;
static BOOL                 aborted ;

int main()
    {
    static void (* const fractals[])(void) =
        {
        BarnsleyFernFractal,
        MandelbrotSetFractal,
        JuliaSetFractal
        } ;

    InitializeHardware(HEADER, "Lab 5B: Pixels, Fonts & Fractals") ;
    if (!SanityChecksOK()) return 0 ;
    ChromArtInitialize() ;
    for (int fractal = 0;; fractal = (fractal + 1) % ITEMS(fractals))
        {
        (*fractals[fractal])() ;
        WaitForPushButton() ;
        }
    }

static void BarnsleyFernFractal(void)
    {
    const int   LIMIT   = 100000 ;
    const float X_ZOOM  = 88 ;
    const float Y_ZOOM  = 112 ;
    float zoom, inc ;

    aborted = FALSE ;
    zoom = 1.0 ;
    inc = 0.25 ;
    while (TRUE)
        {
        uint32_t timeout = GetTimeout(100) ;
        float x, y ;

        memset(frame_pixels, INDEX_RED, sizeof(frame_pixels)) ;
        FractalTitle("Barnsley Fern") ;

        x = y = 0.0 ;
        for (int i = 0; i < LIMIT; i++)
            {
            int r = GetRandomNumber() % 100 ;
            float newX, newY ;
            unsigned pxlX, pxlY ;

            if (r <= 1)
                {
                newX = 0.00 ;
                newY = 0.16 * y ;
                }
            else if (r <= 8)
                {
                newX = 0.20 * x - 0.26 * y ;
                newY = 0.23 * x + 0.22 * y + 1.6 ;
                }
            else if (r <= 15)
                {
                newX = -0.15 * x + 0.28 * y ;
                newY =  0.26 * x + 0.24 * y + 0.44 ;
                }
            else
                {
                newX =  0.85 * x + 0.04 * y ;
                newY = -0.04 * x + 0.85 * y + 1.6 ;
                }

            x = newX ;
            y = newY ;

            pxlY = YSIZE - (int) (y * ((YSIZE*zoom)/Y_ZOOM)) ;
            if (pxlY > YSIZE) continue ;
            pxlX = XSIZE/2 + (int) (x * ((XSIZE*zoom)/X_ZOOM)) ;
            if (pxlX > XSIZE) continue ;
            WritePixel(pxlX, pxlY, INDEX_GRN, frame_pixels) ;
            if (Aborted()) return ;
            }

        ChromArtXferFrameBuffer(screen_pixels, frame_pixels) ;
        zoom += inc ;
        if (zoom >= 26.0) inc = -0.25 ;
        if (zoom <=  1.0) inc = +0.25 ;
        WaitForTimeout(timeout) ;
        ChromArtWaitForDMA() ;
        }
    }

static void MandelbrotSetFractal(void)
    {
    const int   limit   = 18 ;
    const float X_ZOOM  = 1.30 ;
    const float Y_ZOOM  = 0.75 ;
    const float X_OFF   = -0.5 ;
    const float Y_OFF   = 0.0 ;
    CLR_INDEX clroff ;

    memset(frame_pixels, INDEX_RED, sizeof(frame_pixels)) ;
    FractalTitle("Mandelbrot Set") ;

    aborted = FALSE ;
    clroff = 0 ;
    while (TRUE)
        {
        uint32_t timeout = GetTimeout(200) ;
        for (int y = 0; y < YSIZE; y++)
            {
            for (int x = 0; x < XSIZE; x++)
                {
                float py = Y_OFF + (y - YSIZE/2) * (2.0 / (Y_ZOOM * YSIZE)) ;
                float px = X_OFF + (x - XSIZE/2) * (3.0 / (X_ZOOM * XSIZE)) ;
                CLR_INDEX clridx ;
                unsigned iter ;
                float zx, zy ;

                zx = px ; zy = py ;
                for (iter = 0; iter < limit; iter++)
                    {
                    float zxSquared = zx*zx ;
                    float zySquared = zy*zy ;

                    if (zxSquared + zySquared > 4.0) break ;

                    zy = py + 2.0*zx*zy ;
                    zx = px + zxSquared - zySquared ;
                    if (Aborted()) return ;
                    }

                clridx = (clroff + (255*iter)/limit) % 255 ;
                if (iter == limit) clridx = 255 ;
                WritePixel(x, y, clridx, frame_pixels) ;
                }
            }

        ChromArtXferFrameBuffer(screen_pixels, frame_pixels) ;
        clroff += 5 ;
        WaitForTimeout(timeout) ;
        ChromArtWaitForDMA() ;
        }
    }

static void JuliaSetFractal(void)
    {
    const int   limit   = 50;
    const float X_ZOOM  = 1.20 ;
    const float Y_ZOOM  = 0.65 ;
    const float X_OFF   = 0.0 ;
    const float Y_OFF   = 0.0 ;
    int degrees ;

    memset(frame_pixels, INDEX_RED, sizeof(frame_pixels)) ;
    FractalTitle("Julia Set") ;

    aborted = FALSE ;
    degrees = 0 ;
    while (TRUE)
        {
        float radians = (degrees * 2 * PI) / 360.0 ;
        float pX = 0.7885 * cosf(radians) ;
        float pY = 0.7885 * sinf(radians) ;
        uint32_t timeout = GetTimeout(100) ;

        for (int y = 0; y < YSIZE; y++)
            {
            for (int x = 0; x < XSIZE; x++)
                {
                float zy = Y_OFF + (y - YSIZE/2) * (2.0 / (Y_ZOOM * YSIZE)) ;
                float zx = X_OFF + (x - XSIZE/2) * (3.0 / (X_ZOOM * XSIZE)) ;
                int iter ;

                for (iter = 0; iter < limit; iter++)
                    {
                    float zxSquared = zx*zx ;
                    float zySquared = zy*zy ;

                    if (zxSquared + zySquared > 4) break ;

                    zx  = pX + 2.0*zx*zy ;
                    zy  = pY + zySquared - zxSquared ;
                    if (Aborted()) return ;
                    }

                WritePixel(x, y, (255 * iter) / limit , frame_pixels) ;
                }
            }

        ChromArtXferFrameBuffer(screen_pixels, frame_pixels) ;
        degrees = (degrees + 3) % 360 ;
        WaitForTimeout(timeout) ;
        ChromArtWaitForDMA() ;

        }
    }

static BOOL Aborted(void)
    {
    if (PushButtonPressed()) aborted = TRUE ;
    return aborted ;
    }

static CLR_RGB32 HSV2RGB(float hue, float sat, float val)
    {
    static float p, q, t ;
    float * const pRed[] = {&val, &q, &p, &p, &t, &val} ;
    float * const pGrn[] = {&t, &val, &val, &q, &p, &p} ;
    float * const pBlu[] = {&p, &p, &t, &val, &val, &q} ;
    int i, r, g, b ;
    float f, h ;

    h = hue / 60.0 ;    // 0.0 <= h < 6.0
    i = (unsigned) h ;  //   0 <= i < 6
    f = h - (float) i ; //   0 <= f < 6.0

    p = val * (1.0 - sat) ;
    q = val * (1.0 - f*sat) ;
    t = val * (1.0 - (1.0 - f)*sat) ;

    r = (int) (*pRed[i] * 255.0) ;
    g = (int) (*pGrn[i] * 255.0) ;
    b = (int) (*pBlu[i] * 255.0) ;

    return PackRGB(r, g, b) ;
    }

static void ChromArtInitialize(void)
    {
    uint32_t timeout ;
    int color ;

    *AHB1ENR |= (1 << 23) ; // Turn on DMA2D clock
    timeout = GetTimeout(1) ;
    while ((int) (timeout - GetClockCycleCount()) > 0) ;

    // Load color look-up table (CLUT)
    for (color = 0; color < 255; color++)
        {
        float hue = (360.0 * color) / 256 ;
        FG_CLUT[color] = HSV2RGB(hue, 1.0, 1.0) ;
        }
    FG_CLUT[color] = COLOR_BLACK ;
    }

static void ChromArtXferFrameBuffer(CLR_RGB32 *screen_pixels, FRAME frame_pixels)
    {
    DMA2D->NLR      = (WIDTH << 16) | HEIGHT ; 

    DMA2D->OMAR     = (uint32_t) (screen_pixels + XPIXELS*DISPLAY_YOFF + DISPLAY_XOFF) ;
    DMA2D->OOR      = 0 ;   // Output row offset
    DMA2D->OPFCCR   = 0 ;   // Output pixel format ARGB8888.

    DMA2D->FGMAR    = (uint32_t) frame_pixels ; // foreground (source buffer) address.
    DMA2D->FGOR     = 0 ;   // pixels rows are adjacent in source.
    DMA2D->FGPFCCR  = 5 ;   // Source pixel format L8 (8 bits per pixel)

    // Enable PFC (Pixel Format Conversion) and start transfer
    DMA2D->CR       = 0x10001 ;
    }

static void ChromArtWaitForDMA(void)
    {
    // wait until DMA transfer is finished
    while ((DMA2D->CR & 1) != 0)
        {
        // Poll no faster than once every microsecond
        uint32_t timeout = GetClockCycleCount() + CPU_CLOCK_SPEED_MHZ ;
        while ((int) (timeout - GetClockCycleCount()) > 0)
            {
            if (Aborted()) return ;
            }
        }
    }

static CLR_RGB32 PackRGB(int red, int grn, int blu)
    {
#   define  BYTE(pixel)     ((uint8_t *) &pixel)
    static uint32_t pixel = 0xFF000000 ;

    BYTE(pixel)[0] = blu ;
    BYTE(pixel)[1] = grn ;
    BYTE(pixel)[2] = red ;

    return pixel ;
    }

static void PutChar(int x, int y, char ch, sFONT *font)
    {
    uint8_t *pline = BitmapAddress(ch, (uint8_t *) font->table, font->Height, font->Width) ;
    for (int row = 0; row < font->Height; row++)
        {
        uint32_t bits = GetBitmapRow(pline) ;
        for (int col = 0; col < font->Width; col++)
            {
            if ((int32_t) bits < 0) frame_pixels[y + row][x + col] = textColor ;
            bits <<= 1 ;
            }
        pline += (font->Width + 7) / 8 ;
        }
    }

static void PutString(int x, int y, char *str, sFONT *font)
    {
    while (*str != '\0')
        {
        PutChar(x, y, *str++, font) ;
        x += font->Width ;
        }
    }

static void TextColor(CLR_INDEX color)
    {
    textColor = color ;
    }

static void FractalTitle(char *title)
    {
    int width, xpos, ypos ;
    sFONT * const font = &Font16 ;

    TextColor(INDEX_YLW) ;
    width = strlen(title) * font->Width ;
    xpos = (XSIZE - width) / 2 ;
    ypos = HEIGHT - font->Height ;
    PutString(xpos, ypos, title, font) ;
    }

static int SanityChecksOK(void)
    {
    extern void BSP_LCD_SetFont(sFONT *) ;
    sFONT * const font = &Font16 ;
    int ttl, row, col, bugs ;
    unsigned bits,need ;
    char text[100] ;
    void *adrs ;

    BSP_LCD_SetFont(font);

    bugs = 0 ;
    col = 15 ;

    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_WHITE) ;

    ttl = 60 ;
    row = ttl + font->Height + 4 ;
    adrs = BitmapAddress('H', (uint8_t *) Font8.table, Font8.Height, Font8.Width) ;
    bits = *(uint32_t *) adrs ;
    need = 0x487848E8 ;
    if (bits != need)
        {
        DisplayStringAt(col, row, "Character: H") ;
        row += font->Height ;
        DisplayStringAt(col, row, "   Height: 8") ;
        row += font->Height ;
        DisplayStringAt(col, row, "    Width: 5") ;
        row += font->Height ;
        sprintf(text, "  Address: %08X", (unsigned) adrs) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, " Contents: %08X", bits) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, "Should be: %08X", need) ;
        DisplayStringAt(col, row, text) ;
        row += 2*font->Height ;
        bugs++ ;
        }

    adrs = BitmapAddress('/', (uint8_t *) Font24.table, Font24.Height, Font24.Width) ;
    bits = *(uint32_t *) adrs ;
    need = 0x00001800 ;
    if (bits != need)
        {
        DisplayStringAt(col, row, "Character: /") ;
        row += font->Height ;
        DisplayStringAt(col, row, "   Height: 24") ;
        row += font->Height ;
        DisplayStringAt(col, row, "    Width: 17") ;
        row += font->Height ;
        sprintf(text, "  Address: %08X", (unsigned) adrs) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, " Contents: %08X", bits) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, "Should be: %08X", need) ;
        DisplayStringAt(col, row, text) ;
        row += 2*font->Height ;
        bugs++ ;
        }

    WritePixel(100, 100, 0xAA, frame_pixels) ;
    need = 0xAA ;
    if (bugs < 2 && frame_pixels[100][100] != need)
        {
        DisplayStringAt(col, row, "  x (col): 100") ;
        row += font->Height ;
        DisplayStringAt(col, row, "  y (row): 100") ;
        row += font->Height ;
        sprintf(text, " Returned: %02X", frame_pixels[100][100] & 0xFF) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, "Should be: %02X", need) ;
        DisplayStringAt(col, row, text) ;
        row += 2*font->Height ;
        bugs++ ;
        }

    WritePixel(1, 255, 0x55, frame_pixels) ;
    need = 0x55 ;
    if (bugs < 2 && frame_pixels[255][1] != need)
        {
        DisplayStringAt(col, row, "  x (col): 1") ;
        row += font->Height ;
        DisplayStringAt(col, row, "  y (row): 255") ;
        row += font->Height ;
        sprintf(text, " Returned: %02X", frame_pixels[255][1] & 0xFF) ;
        DisplayStringAt(col, row, text) ;
        row += font->Height ;
        sprintf(text, "Should be: %02X", need) ;
        DisplayStringAt(col, row, text) ;
        row += 2*font->Height ;
        bugs++ ;
        }

    if (bugs > 0)
        {
        SetForeground(COLOR_WHITE) ;
        SetBackground(COLOR_RED) ;
        DisplayStringAt(col, ttl, "BitmapAddress Bugs:") ;
        ttl = row + font->Height ;
        row = ttl + font->Height + 4 ;
        }

    SetForeground(COLOR_BLACK) ;
    SetBackground(COLOR_WHITE) ;

    if (bugs != 0)
        {
        SetForeground(COLOR_WHITE) ;
        SetBackground(COLOR_RED) ;
        DisplayStringAt(col, ttl, "PixelAddress Bugs:") ;
        }

    LEDs(!bugs, bugs) ;
    return bugs == 0 ;
    }

static void LEDs(int grn_on, int red_on)
    {
    static uint32_t * const pGPIOG_MODER    = (uint32_t *) 0x40021800 ;
    static uint32_t * const pGPIOG_ODR      = (uint32_t *) 0x40021814 ;
    
    *pGPIOG_MODER |= (1 << 28) | (1 << 26) ;    // output mode
    *pGPIOG_ODR &= ~(3 << 13) ;                 // both off
    *pGPIOG_ODR |= (grn_on ? 1 : 0) << 13 ;
    *pGPIOG_ODR |= (red_on ? 1 : 0) << 14 ;
    }

static uint32_t GetTimeout(uint32_t msec)
    {
    uint32_t cycles = 1000 * msec * CPU_CLOCK_SPEED_MHZ ;
    return GetClockCycleCount() + cycles ;
    }

static void WaitForTimeout(uint32_t timeout)
    {
    while ((int) (timeout - GetClockCycleCount()) > 0 && !aborted)
        {
        if (Aborted()) break ;
        }
    }

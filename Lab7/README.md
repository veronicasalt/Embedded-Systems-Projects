# **ARM Assembly: Bit Manipulation and Bit-Banding (Lab 7E)**

## **Overview**
This repository contains an ARM Assembly implementation of **bit manipulation functions** for handling Tetris-like shapes stored in a 4x4 bit array. The project is part of Lab 7E: Tetris & Gyroscopes and is designed to work with an STM32F429 microcontroller. The implementation replaces two weakly-defined C functions with optimized ARM assembly versions.

##Lea

## **Project Description**
The main program stores the layout of Tetris shapes as a 16-bit variable, treating it as a 4x4 bit matrix. The provided assembly code optimizes two core functions:

- `BOOL GetBit(uint16_t *bits, uint32_t row, uint32_t col)`: Retrieves a bit at a specific row and column.
- `void PutBit(BOOL value, uint16_t *bits, uint32_t row, uint32_t col)`: Modifies a bit at a specific row and column.

The assignment requires two implementations:
1. **Without Bit-Banding** – Standard bitwise operations.
2. **With Bit-Banding** – Optimized for memory-mapped I/O efficiency.

The ARM assembly implementations directly replace their C equivalents in the final executable.

---

## **Key Features**
- **Bit Manipulation with ARM Assembly**: Efficient handling of individual bits using shifts, masks, and memory access.
- **Bit-Banding Optimization**: Utilizes bit-banding for **single-bit atomic access**, improving performance in embedded systems.
- **Tetris Mechanics**: Functions contribute to shape manipulation in a simple **Tetris-like** game, controlled via:
  - **Push Button** (rotates shape).
  - **Gyroscope Tilting** (moves shape left/right).
  - **Fast Tilt** (immediate drop).

---

## **Assembly Code Explanation**
The two functions, `GetBit` and `PutBit`, are implemented in ARM Assembly with bitwise logic.

### **GetBit Function**
- **Calculates** the memory locaiton of the **bit-banding memory mapping**.
- **Retrieves** the specific bit from the calculated address.

### **PutBit Function**
- **Computes** the memory-mapped address for a given bit.
- Writes the specified bit value using bit-banding.
###Score System
- 1 point per placed shape.
- 100 points for clearing a row.
###Controls
- Button Press: Rotate shape 90 degrees clockwise.
- Tilt Left/Right: Move shape horizontally.
- Fast Tilt Down: Drop shape immediately.
- Game over: When no new shapes can enter/

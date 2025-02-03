# **ARM Assembly: Reversi (Othello) Game Optimization (Lab 6E)**

## **Overview**
This repository contains an ARM Assembly implementation of two optimized functions for the **Reversi (Othello) game**, developed as part of **Lab 6E**. The functions replace their original C implementations to improve execution speed and efficiency on an **ARM Cortex-M4** based microcontroller (STM32F429). 

The goal of this project is to enhance **performance-critical operations** in the game by leveraging **efficient parameter passing, function return values, and memory access optimizations** in assembly.

---

## **Project Description**
Reversi (Othello) is a strategy game where two players take turns placing pieces on an 8x8 board. The objective is to **capture and flip** the opponent’s pieces by surrounding them with your own. The player with the most pieces at the end of the game wins.

The main program runs without requiring modifications, but two **performance-critical** functions are replaced with **ARM assembly implementations**:
- `int Between(int min, int value, int max)`: Determines whether a given value is within a specified range.
- `int Count(int cells[], int numb, int value)`: Counts the number of times a specific value appears in an array.

Each function must:
- Use **an IT (If-Then) block** for conditional execution.
- **Avoid PUSH/POP instructions** to optimize function calls.

These functions are defined as "weak" in C, allowing the **linker to automatically replace them** with their optimized assembly counterparts.

---

## **Learning Outcomes**
- ARM Assembly Function Optimization: Understanding performance trade-offs in function calls.
- Efficient Memory Access: Using auto-incrementing to streamline array traversal.
- Calling C Functions from Assembly: Bridging high-level and low-level programming.
- Game Logic in Embedded Systems: Implementing fast decision-making algorithms in real-time.


## **Key Features**
- **Optimized Comparison Logic (`Between`)**: Uses an unsigned arithmetic trick to eliminate unnecessary comparisons, improving execution speed.
- **Efficient Array Traversal (`Count`)**: Utilizes **pre-indexed or post-indexed addressing modes** to optimize memory access.
- **Minimal Instruction Set**: Reduces execution time by avoiding redundant operations.
- **C and Assembly Integration**: Demonstrates how to seamlessly replace C functions with assembly in an embedded system.

---

## **Assembly Code Explanation**
The two functions, `Between` and `Count`, are implemented in ARM Assembly for improved efficiency.

### **Between Function**
- Performs a single comparison instead of two, reducing execution time.
- Uses the IT block to execute conditonal logic efficiently.

### **Count Function**
- Uses auto-increment addressing (`LDR R2, [R0], #4`) for efficient array traversal.
- Minimizes branch instructions to improve execution time.

## Gameplay
- The game runs with green and red pieces on an 8x8 grid.
- Legal moves are indicated in white.
- User plays as green, computer plays as red.

### Game Controls:
- Place a piece: Touch a valid white spot.
- Let the computer move: Press the blue pushbutton.
- Restart game: Press the black pushbutton.
- Winning Condition: The player with the most pieces at the end wins.

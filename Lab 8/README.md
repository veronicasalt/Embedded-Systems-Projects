#Lab 8 Embedded Systems

##Overview
This project is an implementation of a Making Change program using both C and ARM Assembly. The goal is to compute the fewest number of bills and coins needed to represent a given amount of money in U.S. currency.

##Features
- Computes the optimal number of bills ($20, $10, $5, $1) and coins (quarters, dimes, nickels, pennies).
- Implements division without the division instruction and multiplication without the multiply instruction.
- Uses ARM Assembly to replace key functions from the C implementation.
- Designed to run on the 32F429IDISCOVERY board.

##Function details 
('void Bills(uint32_t dollars, BILLS *paper)')
- Computes the number of $20, $10, $5, and $1 bills.
- Uses integer division (/) in the initial implementation.
- Replaces division with reciprocal multiplication and bitwise shifts in assembly.

('void Coins(uint32_t cents, COINS *coins)')
- Computes the number of quarters, dimes, nickels, and pennies.
- Uses integer division (/) in the initial implementation.
- Optimized in assembly to remove division instructions.

##Assembly Constraints
- **No loops** are allowed
- **No direct dvision** instructions (no UDIV)
- Only specific multiplication instructions are permitted (must use optimized shift/add sequences)

##Testing
- The program randomly generates a dollar and cent amount and calculates the optimal change distribution.
- The output is displayed on a touchscreen interface.
- If the calculated total does not match the input amount, an error message is displayed.

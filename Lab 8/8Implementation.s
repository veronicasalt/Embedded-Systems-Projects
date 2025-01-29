/*

    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/
        .syntax     unified
        .cpu        cortex-m4
        .text

// void Bills(uint32_t dollars, BILLS *bills) ;

        .global     Bills
        .thumb_func
        .align

Bills:  // R0 = dollars, R1 = bills

        LDR         R2,=0xCCCCCCCD			// given: Magic Number
        UMULL       R3,R2,R2,R0             // given: R3, R2 store 64-bit output from divide by constant 
        LSRS 		R3, R2, 4				// R2 is divided by 16           
        STR 		R3, [R1]				// Store above result twenties = R3       
        ADD 		R3, R3, R3, LSL 2		// Multiply R3 by 5: multiply R3 by 4, then add R3 = 5(R3) 
        SUB			R0, R0, R3, LSL 2		// Find remaining values/dollars needed after 20 dollar bills   
        B           Common

// void Coins(uint32_t cents, COINS *coins) ;

        .global     Coins
        .thumb_func
        .align

Coins:  // R0 = cents, R1 = coins

        LDR         R2,=0x51EB851F          // given: Magic Number
        UMULL       R3,R2,R2,R0 			// given: R3, R2 store 64-bit output from divide by constant
		LSRS		R3, R2, 3 				// R3 = cents / 25
		STR			R3, [R1]				// Store above result quarters = R3
		ADD 		R2, R3, R3, LSL 1		// Get 3(R3-->quarters) by Multiplying R3 by 2, then adding by R3 = 3(R3)
		LSLS		R2, R2, 3				// Multiply R3-->quarters by 25	
		ADD			R3, R2, R3				// Add to total
		SUB			R0, R0, R3				// Find remaining values/coins needed after quarters	

Common: // R0 = amount, R1 = structure pointer

        LDR         R2,=0xCCCCCCCD          // given: Magic Number
        UMULL       R3,R2,R2,R0        		// given: R3 and R2 are 64-bit output
		LSRS 		R3, R2, 3 				// Dividing R3 by 8
		STR			R3, [R1,4]				// Storing R3, 4 bits over to not overwrite anything
		LSLS 		R2, R3, 3 				// Multiply R3 by 8 in R2
		ADD 		R3, R2, R3, LSL 1 		// Add 2 to R3 to get 10
		SUB			R0, R0, R3				// Subtract to find remaining
        
		LDR         R2,=0xCCCCCCCD
        UMULL       R3, R2,R2,R0
		LSRS		R3, R2, 2 				// Divide R2 by 4
		STR			R3, [R1, 8]				// Storing 8 bits over to not overwrite anything
		ADD 		R3, R3, R3, LSL 2		// To get 5, Multiply R3 by 4, Add 4(R3) + 1  = 5R3
		SUB			R0, R0, R3	 			// Subtract to get remaining values needed to be placed in 1s
		STR			R0, [R1,12]				// Store remaining 1s, no function needed to compute 1s
					
        BX          LR

        .end

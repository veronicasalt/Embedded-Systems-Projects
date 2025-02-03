/*
    This code was written to support the book, "ARM Assembly for Embedded Applications",
    by Daniel W. Lewis. Permission is granted to freely share this software provided
    that this notice is not removed. This software is intended to be used with a run-time
    library adapted by the author from the STM Cube Library for the 32F429IDISCOVERY 
    board and available for download from http://www.engr.scu.edu/~dlewis/book3.
*/
        .syntax         unified
        .cpu            cortex-m4
        .text

// int Between(int min, int value, int max) ;

        .global         Between
        .thumb_func

        .align
Between:        // R0 = min, R1 = val, R2 = max
        	// R2 = max - min
        	// R1 = val - min
        	// (val - min) <= (max - min)?
        	//
		//
		//
        BX              LR

// int Count(int cells[], int numb, int value) ;

        .global        Count
        .thumb_func

        .align
Count:                 		// R0 = cells, R1 = numb, R2 = value
        	 		// count (R3) <-- 0
        	 		// R1 <-- &cells[numb]
Loop:
       		 CMP R0, R1	// done?
        	 		// Yes: return
        	 		// R12 = *cells++
        	 		//
        	 		//
        	 		// if (cells[numb] == value) count++
                 B               Loop
Done:
        	 		// return count
        	 BX              LR

        .end


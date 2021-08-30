/*mbed simple keypad library, using polling 

	 Copyright (c) 2015 Rune Langøy

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef KEYPAD_H
#define KEYPAD_H

#include "DigitalIn.h"
#include "BusOut.h"

namespace mbed{

const char NO_KEY = '\0';
#define KEY_RELEASED NO_KEY
   
const char keys[16] = {'1','2','3','A',
                      '4','5','6','B',
                      '7','8','9','C',
                      '*','0','#','D'};
 /**
 * An simple polling-based interface to read a 4x4 keypad.
 *
 * The function getKey() reads  the index of the pressed key 
 * and returns the letter of the pressed key 
 *
 *  This work is a derivative of the works done by:
 *    Dimiter Kentri in 2010 https://developer.mbed.org/users/DimiterK/code/keypad/
 *  and
 *    Yoong Hor Meng in 2012 https://developer.mbed.org/users/yoonghm/code/keypad/
 *
 * Example:
 * @code
 * #include "mbed.h"
 * #include "keypad.h"
 *	  
 * Serial pc(USBTX, USBRX); 
 * Keypad keypad(D3,D4,D5,D6,D7,D8,D9,D10);
 * // Keypad keypad( PC_3,PC_2,PA_0,PA_1,PA_4,PB_0,PC_1,PC_0 ); // Tested on  Nucleo303RE card
 * 					
 * int main(void)
 * {
 *  keypad.enablePullUp();
 *  char key;
 *  pc.printf("Please touch a key on the keypad\r\n");
 *  while(1)
 *  { 
 *	key = keypad.getKey();	
 * 	if(key != KEY_RELEASED)
 *	{
 *	   pc.printf("%c\r\n",key);
 *	   wait(0.2);
 *	 }
 *  }
 * }	
 * @endcode

 */
                     
class Keypad{    
    public:
    /**  @brief Create a 4x4 (col, row) or 4x4 keypad interface\n
     * \n
     * @table
     *          | Col0 | Col1 | Col2 | Col3   \n
     *   -------+------+------+------+-----   \n
     *   Row 0  |   x  |   x  |   x  |  x     \n
     *   Row 1  |   x  |   x  |   x  |  x     \n
     *   Row 2  |   x  |   x  |   x  |  x     \n
     *   Row 3  |   x  |   x  |   x  |  x     \n
     * @endtable
     *
     *  @param col<0..3>     Row data lines
     *  @param row<0..3>     Column data lines
     */
        Keypad(PinName col0, PinName col1, PinName col2, PinName col3, PinName row0,PinName row1, PinName row2, PinName row3);
    
       /** @brief  Returns the letter of the pressed key \n
        *      if no walid key was pressed\n
        *      the value -1 is returned\n
        *          
        *  @return char - The pressed character or -1 if no char was pressed
        */
        char getKey();
        
        /** @brief Detects if any key was pressed   
        *          
        *   @return bool - true is returned if any key is pressed
        */
        bool getKeyPressed();
        /**  Enables internal PullUp resistors on the coloums pins 
        *          
        *  @return void
        */
        void enablePullUp();

    protected:
        BusIn  _cols;    
        BusOut _rows;
         /** @brief return the index value \n
        *      representating the pressed key \n 
        *      if no walid key was pressed \n
        *      the value -1 is returned
        *          
        *  @return int 
        */
        int getKeyIndex();
       
};

}
#endif

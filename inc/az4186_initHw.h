/*
 * az4186_initHw.h
 *
 *  Created on: Dec 28, 2016
 *      Author: michele
 */

#ifndef AZ4186_INITHW_H_
#define AZ4186_INITHW_H_

unsigned disableIRQ(void);
unsigned restoreIRQ(unsigned vic);
unsigned enableIRQ(void);
void v_reset_az4186(void);

#endif /* AZ4186_INITHW_H_ */

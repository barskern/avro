/******************************************************************************
 * File:             common.h
 *
 * Author:           Ole Martin Ruud
 * Created:          01/20/21
 * Description:      Common helpers and functions.
 *****************************************************************************/

#ifndef AVRO_COMMON_H
#define AVRO_COMMON_H

/* Bit Operation macros */
#define sbi(b,n) ((b) |= (1<<(n)))          /* Set bit number n in byte b */
#define cbi(b,n) ((b) &= (~(1<<(n))))       /* Clear bit number n in byte b   */
#define fbi(b,n) ((b) ^= (1<<(n)))          /* Flip bit number n in byte b    */
#define rbi(b,n) ((b) & (1<<(n)))           /* Read bit number n in byte b    */

#endif /* ifndef AVRO_COMMON_H */

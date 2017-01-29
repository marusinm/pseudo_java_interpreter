/* *************************** stack_test.h ********************************* */
/* Soubor:              stack_test.h                                          */
/* Datum:               prosinec.2016                                         */
/* Predmet:             Formalni jazyky a prekladace (IFJ)                    */
/* Projekt:             Implementace interpretu jazyka IFJ16                  */
/* Varianta zadani:                                                           */
/* Titul,Autori, login:         Javorka Martin        xjavor18                */
/*                              Marušin Marek         xmarus08                */
/*                              Utkin Kirill          xutkin00                */
/*                              Mrva Marián           xmrvam01                */
/*                              Shapochkin Victor     xshapo00                */
/* ************************************************************************** */

#include "stack.h"
void stackPrint( tPrecStack* ptrstack);
void use_stack_init ( tPrecStack* ptrstack );
void use_stack_push ( tPrecStack* ptrstack, int entry, int expression );
void use_stack_top ( tPrecStack* ptrstack);
void customPrintDataType(int cislo);
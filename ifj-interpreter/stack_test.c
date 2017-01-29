/* *************************** stack_test.c ********************************* */
/* Soubor:              stack_test.c                                          */
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

#include <stdlib.h>
#include "stack.h"
#include "stack_test.h"
#include "precedence.h"
#include "scanner.h"
#include "ial.h"

//tStack* ptrstack;

void customPrintDataType(int cislo){
    switch (cislo){
        case valueType_int:
            printf("int");
            break;
        case valueType_string:
            printf("string");
            break;
        case valueType_double:
            printf("double");
            break;
        case valueType_boolean:
            printf("boolean");
            break;
        default:
            printf("undefined type");
            break;
    }
}

void stackPrint( tPrecStack* ptrstack) {
//
//    int maxi = ptrstack->top;
//    printf ("--- BOTTOM [ ");
//    for ( int i=0; i<=maxi; i++ ){
////        printf("expr: %d ", (ptrstack->arr[i])->expression);
////        printf("entry: %d || ", (ptrstack->arr[i])->entry);
////        printf("%d || ", (ptrstack->arr[i])->entry);
//        customPrintState((ptrstack->arr[i])->entry);
////        printf("{%s}", (ptrstack->arr[i])->token.data);
//
//        if ((ptrstack->arr[i])->item != NULL){
////            printf ("(%s,%d)",(ptrstack->arr[i])->item->key,(ptrstack->arr[i])->item->data);
//            printf ("(%s)",(ptrstack->arr[i])->item->key);
////            printf ("[%d]",(ptrstack->arr[i])->item->data->value.constant.type);
//            printf ("[");
//            customPrintDataType((ptrstack->arr[i])->item->data->value.constant.type);
//            printf ("]");
//
////            if ((ptrstack->arr[i])->item->data->value.constant.type == valueType_int)
////                printf("#%d#", (ptrstack->arr[i])->item->data->value.constant.value.t_int);
////            else if ((ptrstack->arr[i])->item->data->value.constant.type == valueType_double)
////                printf("#%f#", (ptrstack->arr[i])->item->data->value.constant.value.t_double);
////            else if ((ptrstack->arr[i])->item->data->value.constant.type == valueType_string)
////                printf ("#%s#",(ptrstack->arr[i])->item->data->value.constant.value.str);
//
//
////            printf ("(%d)",(ptrstack->arr[i])->item->data);
//        }
//
//
//        printf(" @@@ ");
//    }
//    printf (" ] TOP ---\n");
}

void use_stack_init ( tPrecStack* ptrstack ) {
    stackInit( ptrstack );
}

//void use_stack_push ( tStack* ptrstack, int entry, int expression ) {
//    stackPush( ptrstack, entry, expression );
//}

void use_stack_top ( tPrecStack* ptrstack) {

    tData data;
    stackTop( ptrstack, &data );
//
//    printf("expr: %d ", data.token.state);
//    printf("entr: %d ", data.entry);
}
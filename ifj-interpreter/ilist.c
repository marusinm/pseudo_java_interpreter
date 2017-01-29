/* ******************************** ilist.c ********************************* */
/* Soubor:              ilist.c                                               */
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

#include "ilist.h"
#include "garb.h"
#include "ial.h"


// funkcia inicializuje zoznam instrukcii
void listInit(tListOfInstr *L) {
    L->first  = NULL;
    L->last   = NULL;
    L->active = NULL;
}

// funkcia dealokuje zoznam instrukcii
void listFree(tListOfInstr *L) {
    tListItem *ptr;
    while (L->first != NULL) {
        ptr = L->first;
        L->first = L->first->nextItem;
        // uvolnime celu polozku
        free(ptr);
    }
}

// vlozi novu instrukciu na koniec zoznamu
void listInsertLast(tListOfInstr *L, tInstr I) {
    tListItem *newItem;
    newItem = myMalloc(sizeof (tListItem));
    newItem->Instruction = I;
    newItem->nextItem = NULL;
    if (L->first == NULL)
        L->first = newItem;
    else
        L->last->nextItem = newItem;
    L->last=newItem;
}

// zaktivuje prvu instrukciu
void listFirst(tListOfInstr *L) {
    L->active = L->first;
}

// aktivna instrukcia se stane nasledujucou instrukciou
void listNext(tListOfInstr *L) {
    if (L->active != NULL)
        L->active = L->active->nextItem;
}

// nastavime aktivni instrukci podle zadaneho ukazatele
// POZOR, z hlediska predmetu IAL tato funkce narusuje strukturu
// abstraktniho datoveho typu
void listGoto(tListOfInstr *L, void *gotoInstr) {
    L->active = (tListItem*) gotoInstr;
}

// vrati ukazatel na posledni instrukci
// POZOR, z hlediska predmetu IAL tato funkce narusuje strukturu
// abstraktniho datoveho typu
void *listGetPointerLast(tListOfInstr *L) {
    return (void*) L->last;
}

// vracia aktivnu instrukciu
tInstr *listGetData(tListOfInstr *L) {
    if (L->active == NULL) {
        printf("Chyba, zadna instrukce neni aktivni");
        return NULL;
    }
    else
        return &(L->active->Instruction);
}

void print_elements_of_list(tListOfInstr TL)    {

//    tListOfInstr TempList=TL;
//    int CurrListLength = 0;
//    printf("-------------------------------------------------\n");
//    while ((TempList.first!=NULL))    {
//        customPrintInstType(TempList.first->Instruction.instType);
//        printf("\t");
//
//        entry_s *destination = TempList.first->Instruction.addr1;
////        if (destination!=NULL)
////            printf("dest: %s \t",destination->key);
////
////        entry_s *operand1 = TempList.first->Instruction.addr2;
////        if (operand1!=NULL)
////            printf("op1: %s \t",operand1->key);
////
////        entry_s *operand2 = TempList.first->Instruction.addr3;
////        if (operand2!=NULL)
////            printf("op2: %s \n",operand2->key);
////        else
////            printf("\n");
//
//        if ((TempList.first==TL.active) && (TL.active!=NULL))
//            printf("\t <= toto je aktivní prvek ");
//
//        TempList.first=TempList.first->nextItem;
//        CurrListLength++;
//    }
//    printf("\n-------------------------------------------------\n");
}

void customPrintInstType(int instType){
    switch(instType){
        case I_START:
            printf("I_START");
            break;
        case I_STOP:
            printf("I_STOP");
            break;
        case I_READLN_INT:
            printf("I_READLN_INT");
            break;
        case I_READLN_DOUBLE:
            printf("I_READLN_DOUBLE");
            break;
        case I_READLN_STRING:
            printf("I_READLN_STRING");
            break;
        case I_PRINT:
            printf("I_PRINT");
            break;
        case I_LENGTH:
            printf("I_LENGTH");
            break;
        case I_SUBSTR:
            printf("I_SUBSTR");
            break;
        case I_COMPARE:
            printf("I_COMPARE");
            break;
        case I_FIND:
            printf("I_FIND");
            break;
        case I_SORT:
            printf("I_SORT");
            break;
        case I_ADD:
            printf("I_ADD");
            break;
        case I_SUB:
            printf("I_SUB");
            break;
        case I_MUL:
            printf("I_MUL");
            break;
        case I_DIV:
            printf("I_DIV");
            break;
        case I_CONCATENATE:
            printf("I_CONCATENATE");
            break;
        case I_LESS:
            printf("I_LESS");
            break;
        case I_MORE:
            printf("I_MORE");
            break;
        case I_LESS_EQUAL:
            printf("I_LESS_EQUAL");
            break;
        case I_MORE_EQUAL:
            printf("I_MORE_EQUAL");
            break;
        case I_EQUAL:
            printf("I_EQUAL");
            break;
        case I_NOT_EQUAL:
            printf("I_NOT_EQUAL");
            break;
        case I_NOT:
            printf("I_NOT");
            break;
        case I_OR:
            printf("I_OR");
            break;
        case I_AND:
            printf("I_AND");
            break;
        case I_ASSIGN:
            printf("I_ASSIGN");
            break;
        case I_CALL_FUNC:
            printf("I_CALL_FUNC");
            break;
        case I_RETURN:
            printf("I_RETURN");
            break;
        case I_SET_PARAM:
            printf("I_SET_PARAM");
            break;
        case I_JUMP:
            printf("I_JUMP");
            break;
        case I_JUMP_IF_FALSE:
            printf("I_JUMP_IF_FALSE");
            break;
        case I_LABEL:
            printf("I_LABEL");
            break;

        default:
            break;
    }
}
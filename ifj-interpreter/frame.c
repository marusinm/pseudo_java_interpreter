/* ******************************** frame.c  ******************************** */
/* Soubor:              frame.c                                               */
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

#include "frame.h"
#include "interpret.h"
#include "garb.h"
#include "ial.h"
#include "str.h"


#define _DEBUG
// we need function generateConstant


/*  TABULKA SYMBOLU  */

int frameHash(t_table_frame * hashtable_s, char * key){
    int ret_val = 1;
    for (int i = 0; i < strlen(key); i++)
        ret_val += key[i];
    return (ret_val % hashtable_s->size);
}



t_table_frame * frameInit(int size){
    t_table_frame * n_tab;

    if (size<1) return NULL; //tabulka neexistuje


    n_tab = (t_table_frame *) myMalloc(sizeof(t_table_frame) + sizeof(t_table_frame_item *) * size); //alokace pameti pro strukturu tabulky

    if (n_tab == NULL)
        return NULL;


    for(int i=0; i <size; i++) //inicializace prvku tabulky
        n_tab->items[i] = NULL;
    n_tab->size = size;

    return n_tab;
}



int frameLookup(t_table_frame * hashtable_s, char * key){
    t_table_frame_item * list;
    int hash_val = frameHash(hashtable_s, key);

    for(list = hashtable_s->items[hash_val]; list != NULL; list = list->next) {
        if (strcmp(key, list->key) == 0)
            return TAB_SEARCH_OK;
    }
    return TAB_SEARCH_FAIL;

}



int frameInsert(t_table_frame * hashtable_s, char * key,  t_table_frame_item * data){

    int hash_val = frameHash(hashtable_s, key);

    if (frameLookup(hashtable_s, key) == TAB_SEARCH_OK){
        return TAB_INSERT_FOUND;
    }
//    printf("hashtable_s->items[hash_val] %p \n",hashtable_s->items[hash_val]);
    if (hashtable_s->items[hash_val] == NULL){

        data->next = NULL;
    } else {
        data->next = hashtable_s->items[hash_val];
    }

    hashtable_s->items[hash_val] = data;

    return TAB_INSERTED_OK;

}


void frameDelete(t_table_frame * hashtable_s){

    t_table_frame_item *pom1;
    t_table_frame_item *pom2;


    for(int i = 0; i < hashtable_s->size; i++) {
        pom1 = hashtable_s->items[i];
        while(pom1 != NULL) {
            pom2 = pom1;
            pom1 = pom1->next;
            free(pom2->key);
            free(pom2);
        }
    }

    free(hashtable_s->items);//uvolneni tabulky
    free(hashtable_s);
}


void frameDeleteitem(t_table_frame * hashtable_s, char * key){


    int code = frameHash(hashtable_s, key); // vypocitame index do tabulky
    t_table_frame_item * pom1 = hashtable_s->items[code];
    t_table_frame_item * pom2 = pom1;

    while (pom1 != NULL) {
        if (pom1->key == key) {
            if (pom1 != hashtable_s->items[code]) {
                pom2->next = pom1->next;
            }
            else {
                hashtable_s->items[code] = pom1->next;
            }
            free(pom1);
            return;
        }
        pom2 = pom1;
        pom1 = pom1->next;
    }
}

t_table_frame_item* frameSearch(t_table_frame * t_frame, char * key){

    if (t_frame == NULL)
        return NULL;

    int hash_val = frameHash(t_frame, key);

    t_table_frame_item * helplist = t_frame->items[hash_val]; //hashtable_s

//        printf("helplist  === %p \n", helplist);
    if (helplist == NULL) {
//        printf("helplist je Null");
        return NULL;
    }

    while (helplist->next != NULL) {
        helplist = helplist->next;
    }
    return helplist;
}

/*
 *
 * insert to hasht
 *
 */



void createFrame(){
    //printf("cau som v createFrame");
}


/*
 *
 *  Implementation frame stack
 *
 */


void frameStackInit ( tStack* s ) {
/*   ---------
** Provede inicializaci zásobníku - nastaví vrchol zásobníku.
** Hodnoty ve statickém poli neupravujte - po inicializaci zásobníku
** jsou nedefinované.
**
** V případě, že funkce dostane jako parametr s == NULL,
** volejte funkci stackError(SERR_INIT). U ostatních funkcí pro zjednodušení
** předpokládejte, že tato situace nenastane.
*/

    if(s == NULL){
        exit(STACK_ERR);
    }
    else {
        s->top = -1;
    }
}

int frameStackEmpty ( const tStack* s ) {
/*  ----------
** Vrací nenulovou hodnotu, pokud je zásobník prázdný,§§ jinak vrací hodnotu 0.
** Funkci implementujte jako jediný příkaz. Vyvarujte se zejména konstrukce
** typu "if ( true ) b=true else b=false".
*/
    // return true if the top element respond to empty stack "-1"
    return s->top == -1;

}

int frameStackFull ( const tStack* s ) {
/*  ---------
** Vrací nenulovou hodnotu, je-li zásobník plný, jinak vrací hodnotu 0.
** Dejte si zde pozor na častou programátorskou chybu "o jedničku"
** a dobře se zamyslete, kdy je zásobník plný, jestliže může obsahovat
** nejvýše STACK_SIZE prkvů a první prvek je vložen na pozici 0.
**
** Funkci implementujte jako jediný příkaz.
*/

    return s->top == MAX_STACK-1;

}


//void frameStackTop (const tStack* s, char* c ) {

/*   --------
** Vrací znak z vrcholu zásobníku prostřednictvím parametru c.
** Tato operace ale prvek z vrcholu zásobníku neodstraňuje.
** Volání operace Top při prázdném zásobníku je nekorektní
** a ošetřete ho voláním funkce stackError(SERR_TOP).
**
** Pro ověření, zda je zásobník prázdný, použijte dříve definovanou
** funkci stackEmpty.
*/
/*
    if (frameStackEmpty(s)) {
        return;
    }

    else {
        return *c = s->arr[s->top];
    }

}*/



void frameStackPop ( tStack* s ) {
/*   --------
** Odstraní prvek z vrcholu zásobníku. Pro ověření, zda je zásobník prázdný,
** použijte dříve definovanou funkci stackEmpty.
**
** Vyvolání operace Pop při prázdném zásobníku je sice podezřelé a může
** signalizovat chybu v algoritmu, ve kterém je zásobník použit, ale funkci
** pro ošetření chyby zde nevolejte (můžeme zásobník ponechat prázdný).
** Spíše než volání chyby by se zde hodilo vypsat varování, což však pro
** jednoduchost neděláme.
**
*/

    if (!frameStackEmpty(s)){
//        free( (s->arr[s->top])->token );
        free( (s->arr[s->top]) );
        s->top--; // decrement top position after removing top element
    }
}


void frameStackPush ( tStack* s, t_table_frame *newFrameTable ) {
/*   ---------
** Vloží znak na vrchol zásobníku. Pokus o vložení prvku do plného zásobníku
** je nekorektní a ošetřete ho voláním procedury stackError(SERR_PUSH).
**
** Pro ověření, zda je zásobník plný, použijte dříve definovanou
** funkci stackFull.
*/

    if(frameStackFull(s) == 1){
        exit(STACK_ERR);
    }
    else {

        // increment top position
        s->top++; //bug, report to test russian team

        // just push pointer to actual table on top
        s->arr[s->top] = newFrameTable;
    }
}

/*
void stackPrint( tStack* ptrstack) {

    int maxi = ptrstack->top;
    printf ("--- BOTTOM [ ");
    for ( int i=0; i<=maxi; i++ ){

    }
    printf (" ] TOP ---\n");
}
*/


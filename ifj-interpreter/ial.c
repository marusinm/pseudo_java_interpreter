/* ******************************** ial.c *********************************** */
/* Soubor:              ial.c                                                 */
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
#include "ial.h"
#include "stack_test.h"
#include "garb.h"


void sift_down(String* st, int left, int right) {
    int j, i;
    i = left;// index leveho syna
    j = 2 * i;
    int pom = st->str[i];// pomocna promenna 
    bool Cont = (j <= right);//podminka vymeny prvku
    while(Cont){
        if (j < right){
            if(st->str[j] < st->str[j + 1]){
                j++;
            }
        }

        if(pom >= st->str[j]){
            Cont = false;
        }
        else{
            st->str[i] = st->str[j];
            i = j;
            j = 2 * i;//nastaveni praveho syna
            Cont = j <= right;//pokud j vetsi nez prvek right, dalsi syn neexistuje =>  konec pole
        }

    }
    st->str[i] = pom;
}


void heap_sort(String* st){
    
    int i, left, right;
    int pom;

    left = st->length / 2;
    right = st->length - 1;

    for (i = left; i >= 0; i--){
        sift_down(st, i, right);
    }

    for (right = st->length-1; right >= 1; right--){
        pom = st->str[0];
        st->str[0] = st->str[right];
        st->str[right] = pom;
        sift_down(st, 0, right-1);
    }
}

int find(const String* string, const String* subString)
{
    if (string->str == NULL || subString->str == NULL)
        return -1;

    if (subString->str[0] == '\0')
        return 0;

    bool finded = false;
    int subIndex = 0;

    for (int i = 0; i < string->length ; i++)
    {
        if (string->str[i] == subString->str[0] && subString->length == 1)
            return i;

        else if (finded == false && string->str[i] == subString->str[subIndex])
        {
            if (subIndex == (subString->length - 1))
            {
                return i - (subString->length - 1);
            }
            else
            {
                subIndex++;
                continue;
            }
        }

        finded = true;

        if (string->str[i] == subString->str[0])
        {
            finded = false;
            subIndex = 1;
        }
    }

    return -1;
}

/*  TABULKA SYMBOLU  */

int htHash(t_hashtable * hashtable_s, char * key){

    int ret_val = 1;
    for (int i = 0; i < strlen(key); i++)
        ret_val += key[i];
    return (ret_val % hashtable_s->size);
}

t_hashtable * htInit(int size){
    t_hashtable * n_tab;

    if (size<1) return NULL; //tabulka neexistuje


    n_tab = (t_hashtable *) myMalloc(sizeof(t_hashtable) + sizeof(entry_t *) * size); //alokace pameti pro strukturu tabulky

    if (n_tab == NULL)
        return NULL;


    for(int i=0; i <size; i++) //inicializace prvku tabulky
        n_tab->htable[i] = NULL;
    n_tab->size = size;

    return n_tab;
}

int htLookup(t_hashtable * hashtable_s, char * key){
    entry_s * list;
    size_t hash_val = htHash(hashtable_s, key);

    for(list = hashtable_s->htable[hash_val]; list != NULL; list = list->next) {
        if (strcmp(key, list->key) == 0)
            return TAB_SEARCH_OK;
    }
    return TAB_SEARCH_FAIL;

}

int htInsert(t_hashtable * hashtable_s, char * key,  tab_t_data * data){

    entry_t *new_list;
    size_t hash_val = htHash(hashtable_s, key);


    if (htLookup(hashtable_s, key) == TAB_SEARCH_OK){
        return TAB_INSERT_FOUND;
    }

    new_list = myMalloc(sizeof(entry_t));

    if (new_list == NULL){
        return TAB_ERR_INSERT;
    }

    new_list->data = data;
    new_list->key = myMalloc( strlen(key)+1 );
    strcpy(new_list->key, key);

    new_list->next = hashtable_s->htable[hash_val];
    hashtable_s->htable[hash_val] = new_list;

    return TAB_INSERTED_OK;

}

entry_s* htSearch(t_hashtable * hashtable_s, char* key){
    if (hashtable_s == NULL)
        return NULL;

    int hasht;
    entry_s *list, *pom;
    for (hasht = 0; hasht < hashtable_s->size; hasht++) {
        list = hashtable_s->htable[hasht];
        while (list != NULL) {
            pom = list;
            list = list->next;
            int a = strcmp(pom->key,key);
            if(a == 0){
                return pom;
            }else{
                continue;
            }
        }
    }
    return NULL;
}

void htDelete(t_hashtable * hashtable_s){

    entry_t *pom1;
    entry_t *pom2;


    for(int i = 0; i < hashtable_s->size; i++) {
        pom1 = hashtable_s->htable[i];
        while(pom1 != NULL) {
            pom2 = pom1;
            pom1 = pom1->next;
            free(pom2->key);
            free(pom2);
        }
    }

    free(hashtable_s->htable);//uvolneni tabulky
    free(hashtable_s);
}


void htDeleteitem(t_hashtable * hashtable_s, char * key){


    int code = htHash(hashtable_s, key); // vypocitame index do tabulky
    entry_s * pom1 = hashtable_s->htable[code];
    entry_s * pom2 = pom1;

    while (pom1 != NULL) {
        if (pom1->key == key) {
            if (pom1 != hashtable_s->htable[code]) {
                pom2->next = pom1->next;
            }
            else {
                hashtable_s->htable[code] = pom1->next;
            }
            free(pom1);
            return;
        }
        pom2 = pom1;
        pom1 = pom1->next;
    }
}

void table_print(t_hashtable * hashtable_s) {
//    int hasht;
//    printf("<---- hashTable: %p ---->\n",  (void *) hashtable_s);
//    entry_s *list, *pom;
//    for (hasht = 0; hasht < hashtable_s->size; hasht++) {
//        list = hashtable_s->htable[hasht];
//        while (list != NULL) {
//            pom = list;
//            list = list->next;
////            printf("<--  idx: %d, pointer: %p, key: %s, tab_data_type: %d, data_value_typ: %d-->\n", hasht, pom, pom->key, pom->data->type, pom->data->value.constant.type);
////            printf("<--  idx: %d, key: %s, tab_data_type: %d, data_value_typ: %d-->\n", hasht, pom->key, pom->data->type, pom->data->value.constant.type);
//            printf("<--  idx: %d, ", hasht);
//            printf("key: %s, ", pom->key);
//            printf("tab_data_type: ");
//            switch (pom->data->type){
//                case tTypeVariable:
//                    printf("VAR, ");
//                    break;
//                case tTypeFunction:
//                    printf("FUNC, ");
////                    printf("data_value_typ: ");
////                    customPrintDataType(pom->data->value.variable.type);
//                    break;
//                case tTypeClass:
//                    printf("CLASS, ");
//                    break;
//                case tTypeConstant:
//                    printf("CONST, ");
//                    printf("data_value_typ: ");
//                    customPrintDataType(pom->data->value.constant.type);
//                    break;
//                case tTypeFunItem:
//                    printf("F_ITEM, ");
//                    break;
//            }
//            printf("-->\n");
//        }
//    }
}

/* ******************************** parser.c ******************************** */
/* Soubor:              parser.c                                              */
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "scanner.h"
#include "parser.h"
#include "ial.h"
#include "precedence.h"
#include "garb.h"
#include "common.h"
#include "ilist.h"


bool areHelperVariablesInit = false;
bool hasCurrentFunctionReturn = false;

extern  tClassifiedIdentifier * classifiedIdentifier;
extern tToken *token;              //pouzite pre nacitanie getNextTokenFromScanner()
tToken *lastToken = NULL;   //sluzi ako adapter na docasne ulozenie predosleho tokenu. Pokial sa premenna nepouziva je nastavena na NULL
tToken * helperTokenBuffer[BUFFER_SIZE];

t_hashtable * class_symbol_table_ptr = NULL;// pri zmene kontextu nesmieme stratit ukazatal na tabulku symbolov pre triety preto si ho tu ulozime
t_hashtable * function_symbol_table_ptr = NULL;

tListOfInstr * functionHelperParameterInstructionList = NULL;

tCompoundStatementContextStack * compoundStatementContextStack = NULL;

// v niektorych pripadoch si musime pamatat kluc do tabulky symbolov alebo inu pomocnu hodnotu
char * helper_key;// pomocna premmena pre tuto funkciu, ktora si pamata identifikator triedy pre klasifikovany pristup aby sme mohli vyhladavat v tabulke symbolov
char * helper_function_key;
char * current_class_key; //obsahuje nazov triedy v ktorej sa prave nachadza rekurzivny zostup
char * current_function_key; //obsahuje nazov funkcie v ktorej sa prave nachadza rekurzivny zostup
char * helper_var_type; // v niektorych pripadoch si musime pamatat typ premmeej

int function_parameter_counter = 0; // pocitdlo parametrov vo funkcii
entry_s * first_builtin_operand = NULL;
entry_s * second_builtin_operand = NULL;
entry_s * third_builtin_operand = NULL;

//nazvy vstavanych funkcii
//char * builtin_functions[9] = {"print", "compare", "length", "find", "sort", "readInt", "readString", "readDouble ", "substr"};

/**
 * funkcia rozparsuje token ktory reprezentuje klasifikovany pristup
 * vysledok ulozi do pomocnej struktury ClassifiedIdentifier
 */
tClassifiedIdentifier * parseClassifiedIdentifier(tToken * token, tClassifiedIdentifier * classifiedIdentifier) {

//    fprintf(stdout, "current token: %s\n", token->data);
    char *str_helper = myMalloc(strlen(token->data)+1);
    str_helper = memcpy(str_helper, token->data, strlen(token->data)+1);

    classifiedIdentifier->class_identifier = strsep(&str_helper, ".");
    classifiedIdentifier->var_identifier = strsep(&str_helper, ".");
    classifiedIdentifier->isBuiltInFunction = false;

#ifdef _DEBUG_PARSER
//    printf("class: %s, identifier: %s\n",classifiedIdentifier->class_identifier, classifiedIdentifier->var_identifier);
#endif

    bool isBuiltInFunction = false;
    if (strcmp(classifiedIdentifier->class_identifier, "ifj16") == 0){
        char * builtin_functions[9] = {"print", "compare", "length", "find", "sort", "readInt", "readString", "readDouble", "substr"};
        for (int i = 0; i < 9; ++i) {
            if (strcmp(classifiedIdentifier->var_identifier, builtin_functions[i])==0){
                isBuiltInFunction = true;
                break;
            }
        }
    }

    //zistime ci sa nejedna o vstavanu funkciu, pokial ano nemusime kontrolovat ci boli tireda a premenna niekdedy zadeklarovane
    if (isBuiltInFunction == false) {

        //zistime ci v klasifikovanom pristupe bola trieda a funkcia/premenna vytvorene
        //cize skontrolujeme semantiku
        if (parser_phase == second_phase) {
            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
            if (item == NULL) {
                freeAndExit(SEMANTIC_ERROR);
            } else {
                t_hashtable *class_table_for_this_class = item->data->value.class.class_table;
                classifiedIdentifier->classSymbolTable = class_table_for_this_class;
                item = htSearch(class_table_for_this_class, classifiedIdentifier->var_identifier);
                if (item == NULL) {
                    freeAndExit(SEMANTIC_ERROR);
                }
            }
        }
    }else{
#ifdef _DEBUG_PARSER
        printf("jedna sa o vstavanu funkciu\n");
#endif
        classifiedIdentifier->isBuiltInFunction = true;
    }


    return classifiedIdentifier;
}

/**
 * inicializuje kontextovy zasobnik
 */
void contextStackInit ( tCompoundStatementContextStack* s ) {
    if(s != NULL){
        s->top = -1;
        s->arr = myMalloc(sizeof(tState)*STACK_SIZE);
    }else{
        freeAndExit(INTERNAL_ERROR);
    }
}

/**
 * skontroluje ci je zasobnik prazdny
 */
int contextStackEmpty ( const tCompoundStatementContextStack* s ) {
    //Vrati nenulovou hodnotu, pokial je zĂĄsobnĂ­k prĂĄzdnĂ˝, inak vracia hodnotu 0.
    return(s->top > -1 ? 0 : s->top);
}

/**
 * vymaze hodnotu z vrcholu zasobnika
 */
void contextStackPop ( tCompoundStatementContextStack* s ) {
    if (contextStackEmpty(s) == 0){
        s->top = s->top -1;
    }
}

/**
 * vypise pocet lavych zlozenych zatvoriek ktore su ulozene na zasobniku
 */
void contextStackPrint(tCompoundStatementContextStack* s ){
    printf("stack %d \n", s->top+1);
}

int contextStackFull(const tCompoundStatementContextStack* s) {

    // return true if the top element respond to full stack
    return s->top == (STACK_SIZE - 1);
}


/**
* vlozi hodnotu na vrchol zasobika
*/
void contextStackPush ( tCompoundStatementContextStack* s, tState state ) {
    if (contextStackFull(s) == 0){
        s->top =  s->top + 1; //navysime vrchol zasobnika
        s->arr[s->top] = state; //vlozime char
    }else{
        freeAndExit(INTERNAL_ERROR);
    }
}

void printLocalBuffer(){
    printf("----------------\n");
    for (int j = 0; j <  BUFFER_SIZE; ++j) {
        if (helperTokenBuffer[j] != NULL){
            printf("token in buffer: %s \n",helperTokenBuffer[j]->data);
        }
    }
    printf("----------------\n");
}

void insertIntoHelperBuffer(tToken *saved_token){

#ifdef _DEBUG_BUFFER
    printf("insert do lokalneho bufferu\n");
#endif

    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (helperTokenBuffer[i] == NULL){
            //TODO: mozno vznika memory leak
            tToken * helper_token = myMalloc(sizeof(tToken));
            helper_token->data = myMalloc(strlen(saved_token->data)+1);
//            helperTokenBuffer[i] = saved_token;
            memcpy(helper_token->data,saved_token->data, strlen(saved_token->data)+1);
            helper_token->state = saved_token->state;
            helper_token->allocate_mem = saved_token->allocate_mem;
            helper_token->num_of_chars = saved_token->num_of_chars;
            helperTokenBuffer[i] = helper_token;

            break;
        }else if (i == BUFFER_SIZE - 1){ // pokial je buffer plny (ani posledna polozka sa nerovna NULL)
            freeAndExit(INTERNAL_ERROR); // nepodarilo sa najst volnu polozku v buffry hrozilo by pretecenie, vraciame neuspesny return code
        }
    }

#ifdef _DEBUG_BUFFER
    printLocalBuffer();
#endif
}


void initParserBuffer(){

#ifdef _DEBUG_BUFFER
    printf("inicializacia lokalneho bufferu\n");
#endif

    for(int i = 0; i < BUFFER_SIZE; i++){
        helperTokenBuffer[i] = NULL;
    }
}

void initHelperVariables(){
    //premenne sa budu realokovat
    helper_key = myMalloc(sizeof(char));
    helper_function_key = myMalloc(sizeof(char));
    current_class_key = myMalloc(sizeof(char));
    current_function_key = myMalloc(sizeof(char));
    helper_var_type = myMalloc(sizeof(char));

    //functionHelperParameterInstructionList = myMalloc(sizeof(tListOfInstr));

    compoundStatementContextStack = (tCompoundStatementContextStack*) myMalloc(sizeof(tCompoundStatementContextStack));
    contextStackInit(compoundStatementContextStack);

    createIfj16(); //vytvorime v tabulke symbolov zaznam pre triedu vstavanych funkcii

    areHelperVariablesInit = true;
}

/**
 * funkcia naplni tabulku symbolov triedu ifj16 so vsetkymi potebnymi funkciami a ich parametrami
 */
void createIfj16(){

    //najskor vytvorime triedu ifj16 v tabulke symbolov
    table_data_value dataValue;
    dataValue.class.has_def = true;
    dataValue.class.class_table = htInit(71);
    tab_t_data * data = myMalloc(sizeof(tab_t_data));
    data->type = tTypeClass;
    data->value = dataValue;
    htInsert(symbol_table, "ifj16", data);

#ifdef _DEBUG_PARSER
    table_print(symbol_table);
#endif

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // potom zacneme postupne tvorit funkcie triedy ifj16 podla zadania
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    t_hashtable * ifj16_table = htSearch(symbol_table, "ifj16")->data->value.class.class_table;


    createBuildInPrint(ifj16_table);
    createBuildInReadInt(ifj16_table);
    createBuildInReadDouble(ifj16_table);
    createBuildInReadString(ifj16_table);
    createBuildInLenght(ifj16_table);
    createBuildInSort(ifj16_table);
    createBuildInSubstr(ifj16_table);
    createBuildInCompare(ifj16_table);
    createBuildInFind(ifj16_table);

    return;
}

/**
 * vytorenie funkcie print v tabulke symbolov pre triedu ifj16
 */
void createBuildInPrint(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_void;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData->type = tTypeVariable;
        ArgumentData->value = ArgumentValue1;

        htInsert(FunctionTable, "term", ArgumentData);
    }

    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 1;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "print",Data);

    //potom vlozime este do listu parametrov danej funkcie parameter
    entry_s *item_ptr = htSearch(ifj16_table, "print"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item = myMalloc(sizeof(entry_s));
    param_item->data = ArgumentData;
    param_item->key = NULL;
    param_item->next = NULL;

//    entry_s * tmp;
//    if(item_ptr->data->value.funkce.first_par == NULL){
        item_ptr->data->value.funkce.first_par = param_item;
//    }else{
//        tmp = item_ptr->data->value.funkce.first_par;
//        while(tmp->next != NULL){
//            tmp = tmp->next;
//        }
//        tmp->next = param_item;
//    }
#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie readInt v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadInt(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_int;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 0;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "readInt",Data);

#ifdef _DEBUG_PARSER
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie readDouble v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadDouble(t_hashtable * ifj16_table){


    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_double;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 0;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "readDouble",Data);

#ifdef _DEBUG_PARSER
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie readString v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadString(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_string;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 0;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "readString",Data);

#ifdef _DEBUG_PARSER
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie lenght v tabulke symbolov pre triedu ifj16
 */
void createBuildInLenght(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_int;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData->type = tTypeVariable;
        ArgumentData->value = ArgumentValue1;

        htInsert(FunctionTable, "term", ArgumentData);
    }

    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 1;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "length",Data);

    //potom vlozime este do listu parametrov danej funkcie parameter
    entry_s *item_ptr = htSearch(ifj16_table, "length"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item = myMalloc(sizeof(entry_s));
    param_item->data = ArgumentData;
    param_item->key = NULL;
    param_item->next = NULL;

//    entry_s * tmp;
//    if(item_ptr->data->value.funkce.first_par == NULL){
        item_ptr->data->value.funkce.first_par = param_item;
//    }else{
//        tmp = item_ptr->data->value.funkce.first_par;
//        while(tmp->next != NULL){
//            tmp = tmp->next;
//        }
//        tmp->next = param_item;
//    }
#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie sort v tabulke symbolov pre triedu ifj16
 */
void createBuildInSort(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_string;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData->type = tTypeVariable;
        ArgumentData->value = ArgumentValue1;

        htInsert(FunctionTable, "term", ArgumentData);
    }

    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 1;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "sort",Data);

    //potom vlozime este do listu parametrov danej funkcie parameter
    entry_s *item_ptr = htSearch(ifj16_table, "sort"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item = myMalloc(sizeof(entry_s));
    param_item->data = ArgumentData;
    param_item->key = NULL;
    param_item->next = NULL;

//    entry_s * tmp;
//    if(item_ptr->data->value.funkce.first_par == NULL){
        item_ptr->data->value.funkce.first_par = param_item;
//    }else{
//        tmp = item_ptr->data->value.funkce.first_par;
//        while(tmp->next != NULL){
//            tmp = tmp->next;
//        }
//        tmp->next = param_item;
//    }
#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie substr v tabulke symbolov pre triedu ifj16
 */
void createBuildInSubstr(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_string;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime prvy parameter argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData1 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData1->type = tTypeVariable;
        ArgumentData1->value = ArgumentValue1;

        htInsert(FunctionTable, "term1", ArgumentData1);
    }
    //vlozime druhy parameter argumenty do tabulky symbolov
    table_data_value ArgumentValue2;
    tab_t_data * ArgumentData2 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue2.variable.type = valueType_int;
        ArgumentValue2.variable.is_init = false;
        ArgumentValue2.variable.is_declared = true;

        ArgumentData2->type = tTypeVariable;
        ArgumentData2->value = ArgumentValue2;

        htInsert(FunctionTable, "term2", ArgumentData2);
    }
    //vlozime treti parameter argumenty do tabulky symbolov
    table_data_value ArgumentValue3;
    tab_t_data * ArgumentData3 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue3.variable.type = valueType_int;
        ArgumentValue3.variable.is_init = false;
        ArgumentValue3.variable.is_declared = true;

        ArgumentData3->type = tTypeVariable;
        ArgumentData3->value = ArgumentValue3;

        htInsert(FunctionTable, "term3", ArgumentData3);
    }


    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 3;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "substr",Data);

    //potom vlozime este do listu parametrov danej funkcie jej samotne parametre
    entry_s *item_ptr = htSearch(ifj16_table, "substr"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item1 = myMalloc(sizeof(entry_s));
    param_item1->data = ArgumentData1;
    param_item1->key = NULL;
    param_item1->next = NULL;
    item_ptr->data->value.funkce.first_par = param_item1;

    entry_s * param_item2 = myMalloc(sizeof(entry_s));
    param_item2->data = ArgumentData2;
    param_item2->key = NULL;
    param_item2->next = NULL;
    item_ptr->data->value.funkce.first_par->next = param_item2;

    entry_s * param_item3 = myMalloc(sizeof(entry_s));
    param_item3->data = ArgumentData3;
    param_item3->key = NULL;
    param_item3->next = NULL;
    item_ptr->data->value.funkce.first_par->next->next = param_item3;


#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie compare v tabulke symbolov pre triedu ifj16
 */
void createBuildInCompare(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_int;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime prvy parameter argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData1 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData1->type = tTypeVariable;
        ArgumentData1->value = ArgumentValue1;

        htInsert(FunctionTable, "term1", ArgumentData1);
    }
    //vlozime druhy parameter argumenty do tabulky symbolov
    table_data_value ArgumentValue2;
    tab_t_data * ArgumentData2 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue2.variable.type = valueType_string;
        ArgumentValue2.variable.is_init = false;
        ArgumentValue2.variable.is_declared = true;

        ArgumentData2->type = tTypeVariable;
        ArgumentData2->value = ArgumentValue2;

        htInsert(FunctionTable, "term2", ArgumentData2);
    }


    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 2;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "compare",Data);

    //potom vlozime este do listu parametrov danej funkcie jej samotne parametre
    entry_s *item_ptr = htSearch(ifj16_table, "compare"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item1 = myMalloc(sizeof(entry_s));
    param_item1->data = ArgumentData1;
    param_item1->key = NULL;
    param_item1->next = NULL;
    item_ptr->data->value.funkce.first_par = param_item1;

    entry_s * param_item2 = myMalloc(sizeof(entry_s));
    param_item2->data = ArgumentData2;
    param_item2->key = NULL;
    param_item2->next = NULL;
    item_ptr->data->value.funkce.first_par->next = param_item2;


#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * vytorenie funkcie find v tabulke symbolov pre triedu ifj16
 */
void createBuildInFind(t_hashtable * ifj16_table){

    table_data_value DataValue;
    DataValue.funkce.return_type = valueType_int;
    DataValue.funkce.has_def = true;
    DataValue.funkce.local_t = htInit(71);
    tab_t_data * Data = myMalloc(sizeof(tab_t_data));
    DataValue.funkce.first_par = NULL;


    //vlozime prvy parameter argumenty do tabulky symbolov
    t_hashtable * FunctionTable = DataValue.funkce.local_t;
    table_data_value ArgumentValue1;
    tab_t_data * ArgumentData1 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue1.variable.type = valueType_string;
        ArgumentValue1.variable.is_init = false;
        ArgumentValue1.variable.is_declared = true;

        ArgumentData1->type = tTypeVariable;
        ArgumentData1->value = ArgumentValue1;

        htInsert(FunctionTable, "term1", ArgumentData1);
    }
    //vlozime druhy parameter argumenty do tabulky symbolov
    table_data_value ArgumentValue2;
    tab_t_data * ArgumentData2 = myMalloc(sizeof(tab_t_data));
    {
        //najskor vlozime do tabulky symbolov
        ArgumentValue2.variable.type = valueType_string;
        ArgumentValue2.variable.is_init = false;
        ArgumentValue2.variable.is_declared = true;

        ArgumentData2->type = tTypeVariable;
        ArgumentData2->value = ArgumentValue2;

        htInsert(FunctionTable, "term2", ArgumentData2);
    }


    DataValue.funkce.first_var = NULL;
    DataValue.funkce.ret_val = NULL;
    DataValue.funkce.argcount = 2;

    tListOfInstr * printFunctionInstrList;
    printFunctionInstrList = myMalloc(sizeof(tListOfInstr));
    listInit(printFunctionInstrList);
    DataValue.funkce.instrList = printFunctionInstrList;

    Data->type = tTypeFunction;
    Data->value =DataValue;

    //vlozime builtin fuknciu do tabulky symbolom
    htInsert(ifj16_table, "find",Data);

    //potom vlozime este do listu parametrov danej funkcie jej samotne parametre
    entry_s *item_ptr = htSearch(ifj16_table, "find"); //ako kluc sme pouzili nazov funkcie
    entry_s * param_item1 = myMalloc(sizeof(entry_s));
    param_item1->data = ArgumentData1;
    param_item1->key = NULL;
    param_item1->next = NULL;
    item_ptr->data->value.funkce.first_par = param_item1;

    entry_s * param_item2 = myMalloc(sizeof(entry_s));
    param_item2->data = ArgumentData2;
    param_item2->key = NULL;
    param_item2->next = NULL;
    item_ptr->data->value.funkce.first_par->next = param_item2;


#ifdef _DEBUG_PARSER
    table_print(FunctionTable);
    table_print(ifj16_table);
#endif

}

/**
 * podla toho o aku vstavanu funkciu sa jedna budeme generovat instrukciu danej funkcie
 */
void generateInstructionForBuiltInFunction(tListOfInstr * functionInstructionList){
    if(strcmp(classifiedIdentifier->var_identifier, "print") == 0){
        if(first_builtin_operand != NULL){
            freeAndExit(UNINITILIZE_ERRORS);
        }

        tInstr instr;
        instr.instType = I_PRINT;
        instr.addr1 = NULL;
        instr.addr2 = second_builtin_operand;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "readInt") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(first_builtin_operand->data->value.variable.type != valueType_int){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_READLN_INT;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = NULL;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "readDouble") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(first_builtin_operand->data->value.variable.type != valueType_double){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_READLN_DOUBLE;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = NULL;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "readString") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(first_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_READLN_STRING;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = NULL;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "length") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        //prebehne este typova kontrola
        if(first_builtin_operand->data->value.variable.type != valueType_int){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(second_builtin_operand == NULL || second_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_LENGTH;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = second_builtin_operand;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "sort") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        //prebehne este typova kontrola
        if(first_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(second_builtin_operand == NULL || second_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_SORT;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = second_builtin_operand;
        instr.addr3 = NULL;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "substr") == 0){

        //todo sa bude cele prerabat
//        //prebehne este typova kontrola
//        if(second_builtin_operand->data->value.variable.type != valueType_string){
//            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//        }
//        if(third_builtin_operand->data->value.variable.type != valueType_int){
//            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//        }
//        if(third_builtin_operand->data->value.variable.type != valueType_int){
//            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//        }
//
//        tInstr instr;
//        instr.instType = I_SUBSTR;
//        instr.addr1 = first_builtin_operand;
//        instr.addr2 = second_builtin_operand;
//        instr.addr3 = third_builtin_operand;
//        listInsertLast(functionInstructionList, instr);
//        function_parameter_counter= 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "compare") == 0) {
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        //prebehne este typova kontrola
        if(first_builtin_operand->data->value.variable.type != valueType_int){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if (second_builtin_operand == NULL || second_builtin_operand->data->value.variable.type != valueType_string) {
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
//        if (third_builtin_operand == NULL || third_builtin_operand->data->value.variable.type != valueType_string) {
//            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//        }

        tInstr instr;
        instr.instType = I_COMPARE;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = second_builtin_operand;
        instr.addr3 = third_builtin_operand;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter = 0;

    }else if(strcmp(classifiedIdentifier->var_identifier, "find") == 0){
        if(first_builtin_operand == NULL){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        //prebehne este typova kontrola
        if(first_builtin_operand->data->value.variable.type != valueType_int){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(second_builtin_operand == NULL || second_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }
        if(third_builtin_operand == NULL || third_builtin_operand->data->value.variable.type != valueType_string){
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
        }

        tInstr instr;
        instr.instType = I_FIND;
        instr.addr1 = first_builtin_operand;
        instr.addr2 = second_builtin_operand;
        instr.addr3 = third_builtin_operand;
        listInsertLast(functionInstructionList, instr);
        function_parameter_counter= 0;

    }
}


/**
 * funkcia rozparsuje zaciatok programu a rekurzivne rozparsuje triedy programu
 * zaroven sa vnara do parsovania nizsej urovne triedy
 */
int parseProgram(){

    int result_code = OK;

    if(areHelperVariablesInit != true){
        initHelperVariables();
    }
    initParserBuffer();

    token = getNextTokenFromScanner();

#ifdef _DEBUG_PARSER
    fprintf(stdout, "current token: %s\n", token->data);
#endif

    if (token->state == s_eof) {

        //nakonci pred ukoncenim este skonrolujeme ci existuje trieda Main a v nej funkcia run() bez parametrov
        if(parser_phase == second_phase){
            entry_s *item_ptr = htSearch(symbol_table, "Main");
            if(item_ptr == NULL){
#ifdef _DEBUG_PARSER
                printf("semantic error - class Main does not exist\n");
#endif
                freeAndExit(SEMANTIC_ERROR);
            }else{
                class_symbol_table_ptr = item_ptr->data->value.class.class_table;
                item_ptr = htSearch(class_symbol_table_ptr, "run");
                if (item_ptr == NULL){
#ifdef _DEBUG_PARSER
                    printf("semantic error - fun. run() does not exist\n");
#endif
                    freeAndExit(SEMANTIC_ERROR);
                }else{

#ifdef _DEBUG_PARSER
                    printf("instrukcna paska run:\n");
                    print_elements_of_list(*item_ptr->data->value.funkce.instrList);
#endif

                    if(item_ptr->data->value.funkce.first_par != NULL){
#ifdef _DEBUG_PARSER
                        printf("semantic error - fun. run() contains parameters\n");
#endif
                        freeAndExit(SEMANTIC_ERROR);
                    }

                    if(item_ptr->data->value.funkce.return_type != valueType_void){
#ifdef _DEBUG_PARSER
                        printf("semantic error - fun. run() incorrect return value\n");
#endif
                        freeAndExit(SEMANTIC_ERROR);
                    }
                }
            }

            //rovnako skontrolujeme ci sa nepokusil niekto redefinovat triedu ifj16
//            item_ptr = htSearch(symbol_table, "ifj16");
//            if (item_ptr != NULL){
//#ifdef _DEBUG_PARSER
//                printf("semantic error - redefined ifj16\n");
//#endif
//                freeAndExit(SEMANTIC_ERROR);
//            }

        }

#ifdef _DEBUG_PARSER
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!end of file!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif


        result_code = OK; //parsovanie prebehlo v poriadku, program skoncil s eof
    } else {

        //detekcia tokenu class
//            if (token->state == s_class) {
        if (strcmp(token->data, "class")==0) {

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //detekcia tokenu identifier
            if (token->state == s_identifier) {

                current_class_key = myRealloc(current_class_key,strlen(token->data)+1);
                memcpy(current_class_key, token->data, strlen(token->data)+1);

                //zapis triedy do tabulky symbolov

                if (parser_phase == first_phase) {
                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                    if (item_ptr == NULL) {
                        table_data_value dataValue;
                        dataValue.class.has_def = true;
                        dataValue.class.class_table = htInit(71);
                        class_symbol_table_ptr = dataValue.class.class_table;
                        tab_t_data * data = myMalloc(sizeof(tab_t_data));
                        data->type = tTypeClass;
                        data->value = dataValue;

                        htInsert(symbol_table, current_class_key, data);
                    }else{
                        freeAndExit(SEMANTIC_ERROR);
                    }
                }else{
                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                    class_symbol_table_ptr = item_ptr->data->value.class.class_table;
                }

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                //detekcia tokenu {
                if (token->state == s_left_vinculum){
                    result_code = parseClassBody();
                    if (result_code != OK){
                        return SYNTAX_ERROR;
                    }
                }

                //zistujeme ci sa vo funkcii parseClassBody() nezistilo epsilon pravidlo a nevraciam jeden token o uroven vyssie
                if (lastToken != NULL){
                    token = lastToken;
                    lastToken = NULL;
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "returned token: %s\n", token->data);
#endif
                }else{
                    //TODO: tuto vetvu mozno ani nikdy nebudem potrebovat
                    token = getNextTokenFromScanner();

#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif

                }
                //detekcia tokenu }
                if (token->state == s_right_vinculum){

                    if (parser_phase == first_phase){
                        // pokracujeme v rekurzii
                        result_code = parseProgram(first_phase);
                    }else{
                        // pokracujeme v rekurzii
                        result_code = parseProgram(second_phase);
                    }
//                    //pokracujeme v rekurzii
//                    result_code = parseProgram();
                    if (result_code != OK){
                        return (result_code);
                    }
                } else {
                    result_code = SYNTAX_ERROR;
                }
            }else{
                result_code = SYNTAX_ERROR;
            }
        }else{
            result_code = SYNTAX_ERROR;
        }
    }

    return result_code;
}

/**
 * funkcia parsuje telo triedy
 */
int parseClassBody(){

    int result_code = OK;

    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
    fprintf(stdout, "current token: %s\n", token->data);
#endif

    //detekcia tokenu } - zistili sme za sa jedna o epsilon pravidlo a trieda je prazdna, cez premennu lastToken vratime token o uroven vyssie
    if (token->state == s_right_vinculum){
        lastToken = token;
        result_code = OK;
    }else{
        //detekcia tokenu static
//        if (token->state == s_static){
        if (strcmp(token->data, "static")==0){

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //detekcia tokenu <type> - int, String, float
//            if (token->state == s_string || token->state == s_double || token->state == s_int || token->state == s_void){
            if (strcmp(token->data, "String")==0 || strcmp(token->data, "double")==0 || strcmp(token->data, "int")==0|| strcmp(token->data, "void")==0 || strcmp(token->data, "boolean")==0){

                helper_var_type = myRealloc(helper_var_type,strlen(token->data)+1);
                memcpy(helper_var_type,token->data,strlen(token->data)+1);

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                //detekcia tokenu identifier
                if (token->state == s_identifier){

                    helper_key = myRealloc(helper_key,strlen(token->data)+1);
//                    helper_key = myRealloc(helper_key,sizeof(token->data)+1);
                    memcpy(helper_key,token->data,strlen(token->data)+1);

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif

                    if (token->state == s_semicolon){ //detekcia tokenu ; -> znamena to ze bola deklarovana premmena
                        //zapis deklaracie premennej do tabulky symbolov
                        if (parser_phase == first_phase) {
//                            entry_s *item_ptr = htSearch(class_symbol_table_ptr, token->data);
                            entry_s *item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                            if (item_ptr == NULL) {
                                table_data_value dataValue;
                                dataValue.variable.is_declared= true;
                                dataValue.variable.is_init = false;
//                                dataValue.variable.value.t_int = "nieco"
                                if (strcmp(helper_var_type, "String")==0) {
                                    dataValue.variable.type = valueType_string;
                                }else if (strcmp(helper_var_type, "double")==0){
                                    dataValue.variable.type = valueType_double;
                                }else if (strcmp(helper_var_type, "int")==0){
                                    dataValue.variable.type = valueType_int;
                                }else if (strcmp(helper_var_type, "boolean") == 0) {
                                    dataValue.variable.type = valueType_boolean;
                                }else{
                                        freeAndExit(SYNTAX_ERROR);
                                }

                                tab_t_data * data = myMalloc(sizeof(tab_t_data));
                                data->type = tTypeVariable;
                                data->value = dataValue;

                                htInsert(class_symbol_table_ptr, helper_key, data);
                            }else{
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        //zacne sa rekurzivne znova vyhodnocovat dalsi prikaz
                        int result_code = parseClassBody();
                        if (result_code != OK){
                            return result_code;
                        }

                    } else if (token->state == s_equal_sign){ // detekcia tokenu = -> znamena to zaciatok priradenia nejakej premenej

                        entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                        t_hashtable * class_table_for_this_class = item_ptr->data->value.class.class_table;

                        //zapis deklaracie premennej do tabulky symbolov
                        //static identifikatora do tabulky symbolov
                        if (parser_phase == first_phase) {
//                            entry_s *item_ptr = htSearch(class_symbol_table_ptr, token->data);
                            item_ptr = htSearch(class_table_for_this_class, helper_key);
                            if (item_ptr == NULL) {
                                table_data_value dataValue;
                                dataValue.variable.is_declared= true;
                                dataValue.variable.is_init = false;
//                                dataValue.variable.value.t_int = "nieco"
                                if (strcmp(helper_var_type, "String")==0)
                                    dataValue.variable.type = valueType_string;
                                if (strcmp(helper_var_type, "double")==0)
                                    dataValue.variable.type = valueType_double;
                                if (strcmp(helper_var_type, "int")==0)
                                    dataValue.variable.type = valueType_int;
                                if (strcmp(helper_var_type, "boolean")==0)
                                    dataValue.variable.type = valueType_boolean;

                                tab_t_data * data = myMalloc(sizeof(tab_t_data));
                                data->type = tTypeVariable;
                                data->value = dataValue;

                                htInsert(class_table_for_this_class, helper_key, data);
                            }else{
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {
                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeClass;
                            ListOfSymbolsTables->classSymbolTable = class_table_for_this_class;
                            ListOfSymbolsTables->functionSymbolTable = NULL;
                            ListOfSymbolsTables->destinationSymbolTable = class_table_for_this_class;
                            ListOfSymbolsTables->instList = staticInstrList;

                            entry_s * returned_precedence_symbol;
                            returned_precedence_symbol = precedenceCore(second_phase);

                            item_ptr = htSearch(class_table_for_this_class, helper_key);

                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {

                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana

                                ////////////////////////
                                //generovanie instrukcie
                                ////////////////////////
                                tInstr instr;
                                instr.instType = I_ASSIGN;
                                instr.addr1 = item_ptr;
                                instr.addr2 = returned_precedence_symbol;
                                instr.addr3 = NULL;
                                listInsertLast(staticInstrList, instr);

#ifdef _DEBUG_PARSER
                                print_elements_of_list(* ListOfSymbolsTables->instList);
#endif

                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }


                        //skontrolujem posledny nacitany token od precedencnej analyzi
                        if (token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie (funkcia vola sama seba)
                            result_code = parseClassBody();
                            if (result_code != OK){
                                return SYNTAX_ERROR;
                            }
                        }else{
                            return SYNTAX_ERROR;
                        }

                    } else if (token->state == s_left_bracket){ // detekcia tokenu ( -> znamena to zaciatok definicie funkcie

                        //ulozime si nazov aktualnej funkcie pre neskorsie vyuzitie
                        current_function_key = myRealloc(current_function_key,strlen(helper_key)+1);
                        memcpy(current_function_key, helper_key, strlen(helper_key)+1);

                        //zapis deklaracie funkcie do tabulky symbolov
                        if (parser_phase == first_phase) {
                            entry_s *item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                            if (item_ptr == NULL) {
                                table_data_value dataValue;
                                if (strcmp(helper_var_type, "String")==0)
                                    dataValue.funkce.return_type = valueType_string;
                                if (strcmp(helper_var_type, "double")==0)
                                    dataValue.funkce.return_type = valueType_double;
                                if (strcmp(helper_var_type, "int")==0)
                                    dataValue.funkce.return_type = valueType_int;
                                if (strcmp(helper_var_type, "void")==0)
                                    dataValue.funkce.return_type = valueType_void;
                                if (strcmp(helper_var_type, "boolean")==0)
                                    dataValue.funkce.return_type = valueType_boolean;

                                dataValue.funkce.has_def = true;
                                dataValue.funkce.local_t = htInit(71);
                                dataValue.funkce.first_par = NULL;
                                dataValue.funkce.first_var = NULL;
                                dataValue.funkce.ret_val = NULL;
                                dataValue.funkce.argcount = 0;
                                dataValue.funkce.varcount = 0;

                                tListOfInstr * functionInstrList;
                                functionInstrList = myMalloc(sizeof(tListOfInstr));
                                listInit(functionInstrList);
                                dataValue.funkce.instrList = functionInstrList;

                                function_symbol_table_ptr = dataValue.funkce.local_t;

                                tab_t_data * data = myMalloc(sizeof(tab_t_data));
                                data->type = tTypeFunction;
                                data->value = dataValue;

                                htInsert(class_symbol_table_ptr, helper_key, data);
                            } else {
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        result_code = parseDeclarationOfFunctionArguments();
                        if (result_code != OK){
                            return SYNTAX_ERROR;
                        }

                        //ziskame naspat token z urovne nizssie z funkcie parseDeclarationOfFunctionArguments()
                        token = lastToken;
                        lastToken = NULL;
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                        if (token->state != s_right_bracket){
                            return SYNTAX_ERROR;
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        //detekcia tokenu {
                        if (token->state == s_left_vinculum){
                            result_code = parseFunctionBodyList();
                            if (result_code != OK){
                                return SYNTAX_ERROR;
                            }
                        } else{
                            return SYNTAX_ERROR;
                        }

                        token = lastToken;
                        lastToken = NULL;
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif
                        //detekcia ci ziskany token je } -> ukoncenie deklaracie tela funkcie
                        if (token->state == s_right_vinculum) {

                            if (parser_phase == second_phase) {
                                //kontrola ci mala predosla funkcia obsahovat return, robim pracu za interpret
                                if (hasCurrentFunctionReturn == false) {
                                    entry_s *item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                                    if (item_ptr->data->value.funkce.return_type != valueType_void) {
                                        freeAndExit(UNINITILIZE_ERRORS);
                                    }
                                }
                                hasCurrentFunctionReturn = false;
                            }

                            //rekurzivne zavolanie parsovania tela funkcie (funkcia vola sama seba)
                            result_code = parseClassBody();
                            if (result_code != OK){
                                return SYNTAX_ERROR;
                            }

                        }else{
                            result_code = SYNTAX_ERROR;
                        }

                    }else{
                        result_code = SYNTAX_ERROR;
                    }
                } else{
                    result_code = SYNTAX_ERROR;
                }
            } else{
                result_code = SYNTAX_ERROR;
            }
        } else {
            result_code = SYNTAX_ERROR;
        }
    }

    return result_code;
}

/**
 * funkcia parsuje deklaraciu argumentov pri deklaracii funkcie
 */
int parseDeclarationOfFunctionArguments(){

    int result_code = OK;

    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
    fprintf(stdout, "current token: %s\n", token->data);
#endif
    //detekcia tokenu ) -> pouzite epsilon pravidlo, funkcia je bez deklarovanych argumentov
    if (token->state == s_right_bracket) {
        //pri zisteny pravej zatvorky sa moze jednat o prazdy prikaz alebo ukoncenie deklaracii argumentov,
        //z predoslej rekurzie tam vsak mozeme mat ciarku za ktorou nemoze nasledovat prava zatvorka preto nastavime result_code na SYNTAX_ERROR
        if(lastToken != NULL){ //napr. last_token->state == s_comma
            result_code = SYNTAX_ERROR;
        } else {
            lastToken = token;
            result_code = OK;
        }

    }else{

        //detekcia tokenu <type> - int, String, double
//        if (token->state == s_string || token->state == s_double || token->state == s_int) {
        if (strcmp(token->data, "String")==0 || strcmp(token->data, "double")==0 || strcmp(token->data, "int")==0 || strcmp(token->data, "boolean")==0){
            helper_var_type = myRealloc(helper_var_type,strlen(token->data)+1);
            memcpy(helper_var_type,token->data,strlen(token->data)+1);

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //detekcia tokenu identifier
            if (token->state == s_identifier) {

                //zapis deklaracie parametru funkcie do tabulky symbolov
                if (parser_phase == first_phase) {

                    table_data_value dataValue;
                    dataValue.variable.is_declared = true;
                    dataValue.variable.is_init = false;
                    if (strcmp(helper_var_type, "String")==0)
                        dataValue.variable.type = valueType_string;
                    if (strcmp(helper_var_type, "double")==0)
                        dataValue.variable.type = valueType_double;
                    if (strcmp(helper_var_type, "int")==0)
                        dataValue.variable.type = valueType_int;
                    if (strcmp(helper_var_type, "boolean")==0)
                        dataValue.variable.type = valueType_boolean;

                    tab_t_data * data = myMalloc(sizeof(tab_t_data));
                    data->type = tTypeVariable;
                    data->value = dataValue;

                    entry_s * param_item = myMalloc(sizeof(entry_s));
                    param_item->data = data;
//                    param_item->key = token->data;
                    param_item->key = myMalloc(strlen(token->data)+1);
                    strcpy(param_item->key, token->data);
                    param_item->next = NULL;

                    entry_s *item_ptr = htSearch(class_symbol_table_ptr, helper_key); //ako kluc sme pouzili nazov funkcie
                    item_ptr->data->value.funkce.argcount++;

                    entry_s * tmp;
                    if(item_ptr->data->value.funkce.first_par == NULL){
                        item_ptr->data->value.funkce.first_par = param_item;
                    }else{
                        tmp = item_ptr->data->value.funkce.first_par;
                        while(tmp->next != NULL){
                            tmp = tmp->next;
                        }
                        tmp->next = param_item;
                    }


                    // lokalne parametre zapiseme aj do lokalnej tabulky symbolov
                    item_ptr = htSearch(function_symbol_table_ptr, token->data);
                    if (item_ptr == NULL) {
                        htInsert(function_symbol_table_ptr, token->data, data);
                    }else{
                        freeAndExit(SEMANTIC_ERROR);
                    }
                }


                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                if (token->state == s_right_bracket) { //detekcia tokenu ) -> koniec deklaracie argumentov
                    lastToken = token;
                    result_code = OK;
                }else if (token->state == s_comma) { //detekcia tokenu , -> bude nasledovat deklaracia dalieho argumentu
                    lastToken = token;
                    result_code = parseDeclarationOfFunctionArguments(); //rekurzivne zavolanie parsovania dalsich deklaracii argumentov
                    if (result_code != OK){
                        return SYNTAX_ERROR;
                    }
                } else {
                    result_code = SYNTAX_ERROR;
                }
            } else {
                result_code = SYNTAX_ERROR;
            }
        } else {
            result_code = SYNTAX_ERROR;
        }
    }

    return result_code;
}

/**
 * parsovanie tela funkcie
 */
int parseFunctionBodyList(){

    int result_code = OK;

    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
    fprintf(stdout, "current token: %s\n", token->data);
#endif
    //detekcia tokenu } - zistili sme za sa xjedna o epsilon pravidlo a trieda je prazdna, cez premennu lastToken vratime token o uroven vyssie
    if (token->state == s_right_vinculum){
        lastToken = token;
        result_code = OK;
    }else{

        //detekcia tokenu ; -> bol pouzity prazdny prikaz
        if (token->state == s_semicolon){
            //rekurzivne zavolanie parsovania tela funkcie
            result_code = parseFunctionBodyList();
            if (result_code != OK){
                return SYNTAX_ERROR;
            }
        } else if(token->state == s_left_vinculum){ //detekicia tokenu { -> zaciatok zlozeneho prikazu ktory moze existovat len tak sam osebe v tele funkcie

            //pushneme lavu zlozenu zatvorku na zasobnik
            contextStackPush(compoundStatementContextStack, s_left_vinculum);
            // contextStackPrint(compoundStatementContextStack);

            //rekurzivne zavolanie parsovania tela funkcie
            result_code = parseFunctionBodyList();
            if (result_code != OK){
                return SYNTAX_ERROR;
            }

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif

            if (token->state == s_right_vinculum){
                //popneme lavu zatvorku zo zasobnika
                contextStackPop(compoundStatementContextStack);
                //contextStackPrint(compoundStatementContextStack);

                lastToken = token;
                result_code = OK;
            }else{
                return SYNTAX_ERROR;
            }

        } else if (strcmp(token->data, "return") == 0){ // detekcia tokenu return

            hasCurrentFunctionReturn = true;


            //TODO: vyriesit return v semantike
            //TODO: treba sa pytat ci existuje navratova hodnota funkcie v druhej faze zostupu
            //TODO: tym padom as vyriesi aj to ci tam return vobec ma byt alebo nie

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            insertIntoHelperBuffer(token);
            if (token->state == s_semicolon){ // detekcia tokenu ; -> za return moze nasledovat cisto ; bez identifikatora

                initParserBuffer();

                if(parser_phase == second_phase) {
                    entry_s *item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                    if (item_ptr->data->value.funkce.return_type != valueType_void) {
                        freeAndExit(SEMANTIC_ERROR);
                    } else {
                        tInstr instr;
                        instr.instType = I_RETURN;
                        instr.addr1 = item_ptr;
                        instr.addr2 = NULL;
                        instr.addr3 = NULL;

                        listInsertLast(item_ptr->data->value.funkce.instrList, instr);
                    }
                }

                //rekurzivne zavolanie parsovania tela funkcie
                result_code = parseFunctionBodyList();
                if (result_code != OK){
                    return SYNTAX_ERROR;
                }
            }else { //detekcia tokenu s_identifikator -> ktory moze nasledovat za tokenom s_return

                //vratime tokeny z nasho lokalneho bufferu do scannera
                for (int i = 0; i < BUFFER_SIZE; i++) {
                    returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                }
                initParserBuffer(); //reinicializaciou vynulujeme buffer

//                token = getNextTokenFromScanner();
//#ifdef _DEBUG_PARSER
//                fprintf(stdout, "current token: %s\n", token->data);
//#endif
                if(first_phase == parser_phase){
                    precedenceCore(first_phase);
                }else {
                    entry_s *item_ptr = htSearch(class_symbol_table_ptr, current_function_key);

                    //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                    ListOfSymbolsTables->data_type = tTypeFunction;
                    ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                    ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                    ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;
                    ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                    entry_s *returned_precedence_symbol;
                    returned_precedence_symbol = precedenceCore(second_phase);

                    //pred generovanim instrukcie spravime typovu kontrlou
                    if (item_ptr->data->value.funkce.return_type ==
                        returned_precedence_symbol->data->value.variable.type ||
                        (item_ptr->data->value.funkce.return_type == valueType_double &&
                         returned_precedence_symbol->data->value.constant.type == valueType_int)) {

                        tInstr instr;
                        instr.instType = I_RETURN;
                        instr.addr1 = item_ptr;
                        instr.addr2 = returned_precedence_symbol;
                        instr.addr3 = NULL;

                        listInsertLast(item_ptr->data->value.funkce.instrList, instr);

                    }else{
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }


                //nacita sa token ktory vrati precedencka
                if (token->state == s_semicolon) { // detekcia tokenu ; -> za return + identifikatorom musi nasledovat token ;

                    //rekurzivne zavolanie parsovania tela funkcie
                    result_code = parseFunctionBodyList();
                    if (result_code != OK) {
                        return SYNTAX_ERROR;
                    }
                }else {
                    freeAndExit(SYNTAX_ERROR);
                }
            }
        }else if (strcmp(token->data, "if") == 0){ //detekcia tokenu if

            tListItem *tmp;
            tListItem *tmp1;
            tListItem *tmp2;
            tListItem *tmp3;

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //vlozime token do lokalneho parer bufferu
            insertIntoHelperBuffer(token);

            if(token->state == s_left_bracket) {

                //vratime tokeny z nasho lokalneho bufferu do scannera
//                for (int i = 0; i < BUFFER_SIZE; i++) {
//                    returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
//                }
                initParserBuffer(); //reinicializaciou vynulujeme buffer

                if(parser_phase == first_phase) {
                    precedenceCore(first_phase);
                } else if (parser_phase == second_phase) {
                    //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                    ListOfSymbolsTables->data_type = tTypeFunction;
                    ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                    ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                    ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;

                    entry_s * item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                    ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                    entry_s * returned_precedence_symbol;
                    returned_precedence_symbol = precedenceCore(second_phase);

                    if (returned_precedence_symbol->data->value.constant.type == valueType_boolean){
                        //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                        printf("generovanie instrukcie pr if\n");
#endif

                        if (parser_phase == second_phase) {
                            tInstr jmpfInstr;
                            jmpfInstr.instType = I_JUMP_IF_FALSE;
                            jmpfInstr.addr1 = NULL;
                            jmpfInstr.addr2 = returned_precedence_symbol;
                            jmpfInstr.addr3 = NULL;

                            entry_s *item_ptr = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                            table_print(symbol_table);
                            table_print(item_ptr->data->value.class.class_table);
#endif
                            item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                            listInsertLast(item_ptr->data->value.funkce.instrList, jmpfInstr);

                            tmp = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                        }
                    }else{
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }


                //precitame token ktory vrati precedencna tabulka
                if(token->state != s_right_bracket){
                    freeAndExit(SYNTAX_ERROR);
                }
            }else{
                freeAndExit(SYNTAX_ERROR);
            }

            //pushneme lavu zlozenu zatvorku ifu na zasobnik
            contextStackPush(compoundStatementContextStack, s_left_vinculum);
            // contextStackPrint(compoundStatementContextStack);

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "returned token: %s\n", token->data);
#endif

            //skontrolujeme psledny nacitany token z precedencnej analyzi
            if(token->state == s_left_vinculum) { //detekcia tokenu { - zistenie ci po ife nasleduje zloziny prikaz


                //rekurzivne zavolanie parsovania tela funkcie
                result_code = parseFunctionBodyList();
                if (result_code != OK){
                    return SYNTAX_ERROR;
                }


                //ziskame naspat token z urovne nizssie z rekurzie
                token = lastToken;
                lastToken = NULL;
#ifdef _DEBUG_PARSER
                fprintf(stdout, "returned token: %s\n", token->data);
#endif
                //detekcia ci ziskany token je } -> ukoncenie deklaracie tela funkcie
                if (token->state == s_right_vinculum){

                    //popneme lavu zatvorku ifu zo zasobnika
                    contextStackPop(compoundStatementContextStack);
                    //contextStackPrint(compoundStatementContextStack);


                    if (parser_phase == second_phase) {
                        tInstr jmpInstr;
                        jmpInstr.instType = I_JUMP;
                        jmpInstr.addr1 = NULL;
                        jmpInstr.addr2 = NULL;
                        jmpInstr.addr3 = NULL;

                        entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                        listInsertLast(item_ptr->data->value.funkce.instrList, jmpInstr);

                        tmp1 = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                    }

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif
                    if (strcmp(token->data, "else") == 0){
                        //pushneme lavu zlozenu zatvorku ifu na zasobnik
                        contextStackPush(compoundStatementContextStack, s_left_vinculum);
                        // contextStackPrint(compoundStatementContextStack);

                        if (parser_phase == second_phase) {
                            tInstr labelInstr;
                            labelInstr.instType = I_LABEL;
                            labelInstr.addr1 = NULL;
                            labelInstr.addr2 = NULL;
                            labelInstr.addr3 = NULL;

                            entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                            item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                            listInsertLast(item_ptr->data->value.funkce.instrList, labelInstr);

                            tmp2 = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                            tmp->Instruction.addr1 = tmp2;
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //skontrolujeme psledny nacitany token z precedencnej analyzi
                        if(token->state == s_left_vinculum) {//detekcia tokenu { - zistenie ci po ife nasleduje zloziny prikaz
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }


                            //ziskame naspat token z urovne nizssie z rekurzie
                            token = lastToken;
                            lastToken = NULL;
#ifdef _DEBUG_PARSER
                            fprintf(stdout, "returned token: %s\n", token->data);
#endif
                            //detekcia ci ziskany token je } -> ukoncenie deklaracie tela funkcie
                            if (token->state == s_right_vinculum) {

                                //popneme lavu zatvorku ifu zo zasobnika
                                contextStackPop(compoundStatementContextStack);
                                //contextStackPrint(compoundStatementContextStack);

                                if (parser_phase == second_phase) {
                                    tInstr labelInstr2;
                                    labelInstr2.instType = I_LABEL;
                                    labelInstr2.addr1 = NULL;
                                    labelInstr2.addr2 = NULL;
                                    labelInstr2.addr3 = NULL;

                                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                                    item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                                    listInsertLast(item_ptr->data->value.funkce.instrList, labelInstr2);

                                    tmp3 = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                                    tmp1->Instruction.addr1 = tmp3;
                                }

                            }

                            //rekurzivne zavolanie parsovania tela funkcie (funkcia vola sama seba)
                            result_code = parseFunctionBodyList();
                            if (result_code != OK){
                                return SYNTAX_ERROR;
                            }
                        }
                    }else {
                        return SYNTAX_ERROR;
                    }

                }else{
                    result_code = SYNTAX_ERROR;
                }

            }else{
                result_code = SYNTAX_ERROR;
            }

        }else if (strcmp(token->data, "while") == 0){ // detekcia tokenu while


            tListItem *tmp;
            tListItem *tmp1;
            tListItem *tmp2;
            tListItem *tmp3;

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //vlozime token do lokalneho parer bufferu
            insertIntoHelperBuffer(token);

            if(token->state == s_left_bracket) {

                //vratime tokeny z nasho lokalneho bufferu do scannera
//                for (int i = 0; i < BUFFER_SIZE; i++) {
//                    returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
//                }
                initParserBuffer(); //reinicializaciou vynulujeme buffer



                if (parser_phase == second_phase) {

                    tInstr labelInstr;
                    labelInstr.instType = I_LABEL;
                    labelInstr.addr1 = NULL;
                    labelInstr.addr2 = NULL;
                    labelInstr.addr3 = NULL;

                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                    table_print(symbol_table);
                    table_print(item_ptr->data->value.class.class_table);
#endif
                    item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);

//                    printf("madafaka\n");
//                    print_elements_of_list(*item_ptr->data->value.funkce.instrList);

                    listInsertLast(item_ptr->data->value.funkce.instrList, labelInstr);

                    //printf("madafaka\n");
//                    print_elements_of_list(*item_ptr->data->value.funkce.instrList);
//
                    tmp = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                }

                if(parser_phase == first_phase) {
                    precedenceCore(first_phase);
                } else if (parser_phase == second_phase) {
                    //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                    ListOfSymbolsTables->data_type = tTypeFunction;
                    ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                    ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                    ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;

                    entry_s * item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                    ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                    entry_s * returned_precedence_symbol;
                    returned_precedence_symbol = precedenceCore(second_phase);

                    if (returned_precedence_symbol->data->value.constant.type == valueType_boolean){
                        //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                        printf("generovanie instrukcie pre while\n");
#endif

                        tInstr jmpfInstr;
                        jmpfInstr.instType = I_JUMP_IF_FALSE;
                        jmpfInstr.addr1 = NULL;
                        jmpfInstr.addr2 = returned_precedence_symbol;
                        jmpfInstr.addr3 = NULL;

                        entry_s *item_ptr = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                        table_print(symbol_table);
                        table_print(item_ptr->data->value.class.class_table);
#endif
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                        listInsertLast(item_ptr->data->value.funkce.instrList, jmpfInstr);

                        tmp1 = listGetPointerLast(item_ptr->data->value.funkce.instrList);

                    }else{
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }

                //precitame token ktory vrati precedencna tabulka
                if(token->state != s_right_bracket){
                    freeAndExit(SYNTAX_ERROR);
                }
            }else{
                freeAndExit(SYNTAX_ERROR);
            }


            //pushneme lavu zlozenu zatvorku whilu na zasobnik
            contextStackPush(compoundStatementContextStack, s_left_vinculum);
            //contextStackPrint(compoundStatementContextStack);

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "returned token: %s\n", token->data);
#endif

            //skontrolujeme psledny nacitany token z precedencnej analyzi
            if(token->state == s_left_vinculum) {//detekcia tokenu { - zistenie ci po ife nasleduje zloziny prikaz
                //rekurzivne zavolanie parsovania tela funkcie
                result_code = parseFunctionBodyList();
                if (result_code != OK) {
                    return SYNTAX_ERROR;
                }

                if (parser_phase == second_phase) {

                    tInstr jmpInstr;
                    jmpInstr.instType = I_JUMP;
                    jmpInstr.addr1 = NULL;
                    jmpInstr.addr2 = NULL;
                    jmpInstr.addr3 = NULL;

                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                    table_print(symbol_table);
                    table_print(item_ptr->data->value.class.class_table);
#endif
                    item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                    listInsertLast(item_ptr->data->value.funkce.instrList, jmpInstr);

                    tmp2 = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                    tmp2->Instruction.addr1 = tmp;


                    tInstr labelInstr2;
                    labelInstr2.instType = I_LABEL;
                    labelInstr2.addr1 = NULL;
                    labelInstr2.addr2 = NULL;
                    labelInstr2.addr3 = NULL;

                    item_ptr = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                    table_print(symbol_table);
                    table_print(item_ptr->data->value.class.class_table);
#endif
                    item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                    listInsertLast(item_ptr->data->value.funkce.instrList, labelInstr2);

                    tmp3 = listGetPointerLast(item_ptr->data->value.funkce.instrList);
                    tmp1->Instruction.addr1 = tmp3;
                }

                //ziskame naspat token z urovne nizssie z rekurzie
                token = lastToken;
                lastToken = NULL;
#ifdef _DEBUG_PARSER
                fprintf(stdout, "returned token: %s\n", token->data);
#endif
                //detekcia ci ziskany token je } -> ukoncenie deklaracie tela funkcie
                if (token->state == s_right_vinculum){

                    //popneme lavu zatvorku whilu zo zasobnika
                    contextStackPop(compoundStatementContextStack);
                    //contextStackPrint(compoundStatementContextStack);

                    //rekurzivne zavolanie parsovania tela funkcie (funkcia vola sama seba)
                    result_code = parseFunctionBodyList();
                    if (result_code != OK){
                        return SYNTAX_ERROR;
                    }
                }else{
                    result_code = SYNTAX_ERROR;
                }

            }else{
                result_code = SYNTAX_ERROR;
            }
//        }else if (token->state == s_string || token->state == s_double || token->state == s_int){ // detekcia tokenu <type> -> int, String, double
        }else if (strcmp(token->data, "String")==0 || strcmp(token->data, "double")==0 || strcmp(token->data, "int")==0|| strcmp(token->data, "boolean")==0){ // detekcia tokenu <type> -> int, String, double

            ////////////////////////////////////////////////////////////////////////////////////////////////////
            ////DETEKCIA DEKLARACIE IDENTIFIKATORA NA UROVNI TELA FUNKCIE
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            //skontrolujeme ci v tomto bloku kodu mozeme inicializovat premenu
            if(contextStackEmpty(compoundStatementContextStack) == 0){
#ifdef _DEBUG_PARSER
                printf("nepovolena inicializacia v niektorom z blokov kodu");
#endif
                freeAndExit(SYNTAX_ERROR);
            }

            helper_var_type = myRealloc(helper_var_type,strlen(token->data)+1);
            memcpy(helper_var_type,token->data,strlen(token->data)+1);

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            // detekcia tokenu s_identifier
            if (token->state == s_identifier){

                helper_key = myRealloc(helper_key,strlen(token->data)+1);
                memcpy(helper_key,token->data,strlen(token->data)+1);


                helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
                memcpy(helper_function_key,token->data,strlen(token->data)+1);

                //zapis deklaracie lokalnej premennej do tabulky symbolov
                //pytame sa az v druhej faze lebo vo funkcii by mohlo vzniknut situacia ze skor si vyziadam hodnotu premmenej ako bola zadeklarovana
                if (parser_phase == second_phase) {

//                    entry_s *item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);

                    //skontrolujeme redefiniciu funkcie
                    entry_s * redef_test = htSearch(item_ptr->data->value.class.class_table, helper_key);

                    if (redef_test != NULL){
                        if(redef_test->data->type == tTypeFunction){
                            freeAndExit(SEMANTIC_ERROR);
                        }
                    }

                    item_ptr = htSearch(item_ptr->data->value.class.class_table, current_function_key);
                    function_symbol_table_ptr = item_ptr->data->value.funkce.local_t;
                    item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                    if (item_ptr == NULL) {
                        table_data_value dataValue;
                        dataValue.variable.is_declared = true;
                        dataValue.variable.is_init = false;
//                                dataValue.variable.value.t_int = "nieco"
                        if (strcmp(helper_var_type, "String")==0)
                            dataValue.variable.type = valueType_string;
                        if (strcmp(helper_var_type, "double")==0)
                            dataValue.variable.type = valueType_double;
                        if (strcmp(helper_var_type, "int")==0)
                            dataValue.variable.type = valueType_int;
                        if (strcmp(helper_var_type, "boolean")==0)
                            dataValue.variable.type = valueType_boolean;

                        tab_t_data * data = myMalloc(sizeof(tab_t_data));
                        data->type = tTypeVariable;
                        data->value = dataValue;


                        htInsert(function_symbol_table_ptr, helper_key, data);
#ifdef _DEBUG_PARSER
                        table_print(function_symbol_table_ptr);
#endif


                        //kaby sa v dalsich krokoch do premmenej priradovala funkcia musime
                        //poznat do ktoreho operandu priradujeme, toto potom vlozime do instrukcie
                        //////////////////////////////
                        // tvorba operandu
                        //////////////////////////////
                        if(parser_phase == second_phase) {
                            first_builtin_operand = htSearch(function_symbol_table_ptr,helper_key);
                        }

                    }else{
                        freeAndExit(SEMANTIC_ERROR);
                    }
                }

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                if (token->state == s_semicolon){

                    //rekurzivne zavolanie parsovania tela funkcie
                    result_code = parseFunctionBodyList();
                    if (result_code != OK){
                        return SYNTAX_ERROR;
                    }
                }else if (token->state == s_equal_sign){ //detekcia tokenu =

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif

                    //ulozenie tokenu pre pripadny rollback tokenov
                    insertIntoHelperBuffer(token);

                    if (token->state == s_identifier){ //detekcia identifikatora

                        //docasne si zapamatame nazov identifikatora pokial by sa jednalo o klasifikovany pristup
                        helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
                        memcpy(helper_function_key,token->data,strlen(token->data)+1);

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif

                        //ulozenie tokenu pre pripadny rollback tokenov
                        insertIntoHelperBuffer(token);

                        //detekcia tokenu ( -> lava zatvorka
                        if (token->state == s_left_bracket) {

                            //uz vieme ze rolback tokenov nebude
                            //znovuinicializovanim buffer premazeme a nastavime na NULL
                            initParserBuffer();

                            //zistime ci bola funkcia v triede zadefinovana
                            if (parser_phase == second_phase) {
                                //najskor zyskame odkaz na tabulku tabulku symbolov triedy v ktorej sa nachadzame
                                entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                                class_symbol_table_ptr = item_ptr->data->value.class.class_table;

                                //zistime ci trieda obsahuje funkciu s tymto nazvom
                                item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);
                                if (item_ptr == NULL) {
                                    freeAndExit(SEMANTIC_ERROR);
                                }
                            }

                            //vlozime novy token do lokalneho pareser bufferu
                            insertIntoHelperBuffer(token);

                            result_code = parseFunctionParamList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                            //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                            token = lastToken;
                            lastToken = NULL;

#ifdef _DEBUG_PARSER
                            fprintf(stdout, "returned token: %s\n", token->data);
#endif

                            //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                            if (token->state != s_right_bracket) {
                                return SYNTAX_ERROR;
                            }else{
                                if(parser_phase == second_phase){
                                    entry_s *item = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                                    table_print(item->data->value.class.class_table);
#endif
                                    item = htSearch(item->data->value.class.class_table,helper_function_key);

                                    //najdem symbol ciela kam priradujem funkciu
                                    entry_s * dst_item = htSearch(symbol_table, current_class_key);
                                    dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                    dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_key);

                                    tInstr instr;
                                    instr.instType = I_CALL_FUNC;
                                    instr.addr1 = dst_item; // kam funckiu priradujem
                                    instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                                    instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                                    item = htSearch(symbol_table, current_class_key);
                                    item = htSearch(item->data->value.class.class_table, current_function_key);
                                    //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                                    listInsertLast(item->data->value.funkce.instrList, instr);
                                    function_parameter_counter = 0;
                                }
                            }

                            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                            fprintf(stdout, "current token: %s\n", token->data);
#endif
                            //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                            if (token->state == s_semicolon){
                                //rekurzivne zavolanie parsovania tela funkcie
                                result_code = parseFunctionBodyList();
                                if (result_code != OK) {
                                    return SYNTAX_ERROR;
                                }
                            } else{
                                result_code = SYNTAX_ERROR;
                            }

                        }else{ //pokial sa nejedna o lavu zatvorku bude sa jednat pravdepodobne o vyraz a riadenie prenechavame precedencnej tabulke

                            //vratime tokeny z nasho lokalneho bufferu do scannera
                            for (int i = 0; i < BUFFER_SIZE; i++) {
                                if(helperTokenBuffer[i]!=NULL){
#ifdef _DEBUG_BUFFER
                                    printf("kopirujem hodnotu %s \n",helperTokenBuffer[i]->data);
#endif
                                }
                                returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                            }

                            initParserBuffer(); //reinicializaciou vynulujeme buffer
#ifdef _DEBUG_BUFFER
                            printLocalBuffer();
#endif

                            // TODO: tu je volanie precedencnej tabulky
                            if(parser_phase == first_phase) {
                                precedenceCore(first_phase);
                            } else if (parser_phase == second_phase) {
                                //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                                ListOfSymbolsTables->data_type = tTypeFunction;
                                ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                                ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                                ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;

//                                entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                                entry_s * item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                                ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                                entry_s * returned_precedence_symbol;
                                returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                                printf("priradenie po deklaracii helper key %s \n",helper_key);
#endif
                                item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                                //pred generovanim instrukcie spravime typovu kontrlou
                                if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                                   (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                    item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                    //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                    printf("generovanie instrukcie priradenia\n");
#endif
                                    tInstr instr;
                                    instr.instType = I_ASSIGN;
                                    instr.addr1 = item_ptr;
                                    instr.addr2 = returned_precedence_symbol;
                                    instr.addr3 = NULL;

                                    item_ptr = htSearch(symbol_table,current_class_key);
                                    item_ptr = htSearch(item_ptr->data->value.class.class_table,current_function_key);
                                    listInsertLast(item_ptr->data->value.funkce.instrList, instr);

#ifdef _DEBUG_PARSER
                                    print_elements_of_list(*item_ptr->data->value.funkce.instrList);
#endif
                                }else{
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            }

                            //nacitame posledny token z precedencnej analyzi
                            if(token->state == s_semicolon){
                                //rekurzivne zavolanie parsovania tela funkcie
                                result_code = parseFunctionBodyList();
                                if (result_code != OK) {
                                    return SYNTAX_ERROR;
                                }
                            }else {
                                result_code = SYNTAX_ERROR;
                            }
                        }
                    }else if(token->state == s_classified_identifier){

                        ////////////////////////////////////////////////////////////////////////////////////////////////////
                        ////KLASIFIKOVANY PRISTUP NA PRAVEJ STRANE VYRAZU PO DEKLARACII PREMENNEJ
                        ////////////////////////////////////////////////////////////////////////////////////////////////////

                        //ulozenie tokenu pre pripadny rollback tokenov
                        insertIntoHelperBuffer(token);

                        //rozparsujeme klasifikovany pristup a ulozime do pomocnej tabulky symbolov
                        classifiedIdentifier = parseClassifiedIdentifier(token, classifiedIdentifier);

                        helper_key = myRealloc(helper_key,strlen(token->data)+1);
                        memcpy(helper_key,token->data,strlen(token->data)+1);

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        if(token->state == s_semicolon){

                            initParserBuffer(); //vyprazdnime lokalny  buffer po tom co vieme ze ho uz nebudeme potrebovat

                            //skontrolujeme ci druhy identirikator nie je nahodou funkcia
                            if (parser_phase == second_phase) {
                                entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                                t_hashtable * class_table_for_this_class = item->data->value.class.class_table;
                                item = htSearch(class_table_for_this_class, classifiedIdentifier->var_identifier);
                                if(item->data->type == tTypeFunction){
                                    freeAndExit(SEMANTIC_ERROR);
                                }
                            }

                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else if(token->state == s_left_bracket){

                            initParserBuffer(); //vyprazdnime lokalny  buffer po tom co vieme ze ho uz nebudeme potrebovat

                            //skontrolujeme ci druhy identirikator nie je nahodou premenna
                            if (parser_phase == second_phase) {
                                if(classifiedIdentifier->isBuiltInFunction == false) {
                                    entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                                    t_hashtable *class_symbol_table_for_this_class = item->data->value.class.class_table;
                                    item = htSearch(class_symbol_table_for_this_class,
                                                    classifiedIdentifier->var_identifier);
                                    //fixme treba odtestovat ci prejde klasifikovane volanie funkcie aj ked druhy identifikator je variable
                                    if (item->data->type == tTypeVariable) {
                                        freeAndExit(SEMANTIC_ERROR);
                                    }
                                }
                            }

                            result_code = parseFunctionParamList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                            //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                            token = lastToken;
                            lastToken = NULL;

#ifdef _DEBUG_PARSER
                            fprintf(stdout, "returned token: %s\n", token->data);
#endif

                            //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                            if (token->state != s_right_bracket) {
                                return SYNTAX_ERROR;
                            }else{
                                ////////////////////////////////////////
                                //generovanie instrukcie skoku funkcie
                                ////////////////////////////////////////

//                                if (parser_phase == second_phase) {
//                                    //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
//                                    if (classifiedIdentifier->isBuiltInFunction == false) {
//                                        //TODO: vykona sa instrukcia skoku do funkcie
//                                    } else {
////                                        table_print(class_symbol_table_ptr);
////                                        entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
//
//                                        entry_s *item = htSearch(symbol_table, current_class_key);
//                                        item = htSearch(item->data->value.class.class_table, current_function_key);
//
//                                        generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);
//                                    }
//                                }
                                if (parser_phase == second_phase) {
                                    //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
                                    if (classifiedIdentifier->isBuiltInFunction == false) {
                                        //TODO: vykona sa instrukcia skoku do funkcie
                                        entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
#ifdef _DEBUG_PARSER
                                        table_print(item->data->value.class.class_table);
#endif
                                        item = htSearch(item->data->value.class.class_table,classifiedIdentifier->var_identifier);

                                        //najdem symbol ciela kam priradujem funkciu
                                        entry_s * dst_item = htSearch(symbol_table, current_class_key);
                                        dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                        dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_function_key);

                                        tInstr instr;
                                        instr.instType = I_CALL_FUNC;
                                        instr.addr1 = dst_item; // kam funckiu priradujem
                                        instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                                        instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                                        item = htSearch(symbol_table, current_class_key);
                                        item = htSearch(item->data->value.class.class_table, current_function_key);
                                        //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                                        listInsertLast(item->data->value.funkce.instrList, instr);
                                        function_parameter_counter = 0;

                                    } else {
//                                    table_print(class_symbol_table_ptr);
//                                    entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
                                        entry_s *item = htSearch(symbol_table, current_class_key);
                                        item = htSearch(item->data->value.class.class_table, current_function_key);

                                        //najdem symbol ciela kam priradujem funkciu
                                        entry_s * dst_item =  htSearch(symbol_table, current_class_key);
                                        dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                        dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_function_key);
                                        first_builtin_operand = dst_item;

                                        //vlozime instrukciu zavolania vstavanej funkcie na instrukcnu pasku
                                        generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);

                                    }
                                }
                            }

                            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                            fprintf(stdout, "current token: %s\n", token->data);
#endif
                            //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                            if (token->state == s_semicolon){
                                //rekurzivne zavolanie parsovania tela funkcie
                                result_code = parseFunctionBodyList();
                                if (result_code != OK) {
                                    return SYNTAX_ERROR;
                                }
                            } else{
                                result_code = SYNTAX_ERROR;
                            }
                        }else{
                            //TODO: volanie precedencnej analyzi v klasifikovanom pristupe
                            //vratime tokeny z nasho lokalneho bufferu do scannera
                            for (int i = 0; i < BUFFER_SIZE; i++) {
                                returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                            }

                            initParserBuffer(); //reinicializaciou vynulujeme buffer

                            // TODO: tu je volanie precedencnej tabulky
                            if(parser_phase == first_phase) {
                                precedenceCore(first_phase);
                            } else if (parser_phase == second_phase) {

                                entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                                t_hashtable * class_symbol_table_for_this_class = item->data->value.class.class_table;

                                //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                                ListOfSymbolsTables->data_type = tTypeFunction;
                                ListOfSymbolsTables->classSymbolTable = class_symbol_table_for_this_class;
                                ListOfSymbolsTables->functionSymbolTable = NULL;
                                ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;

                                entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                                ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                                entry_s * returned_precedence_symbol;
                                returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                                printf("priradenie po deklaracii - na lavej strane klasifikovany pristup helper key %s \n",helper_key);
#endif
                                item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                                //pred generovanim instrukcie spravime typovu kontrlou
                                if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                                   (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                    item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                    //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                    printf("generovanie instrukcie priradenia\n");
#endif

                                }else{
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            }


                            //nacitame posledny token z precedencnej analyzi
                            if(token->state == s_semicolon){
                                //rekurzivne zavolanie parsovania tela funkcie
                                result_code = parseFunctionBodyList();
                                if (result_code != OK) {
                                    return SYNTAX_ERROR;
                                }
                            }else {
                                result_code = SYNTAX_ERROR;
                            }
                        }

                    }else{
//                        result_code = SYNTAX_ERROR;

                        //najskor zapis deklaracie lokalnej premennej do tabulky symbolov - zapisujeme premennu este na lavej strane vyrazu - helper_key
                        //pytame sa az v druhej faze lebo vo funkcii by mohlo vzniknut situacia ze skor si vyziadam hodnotu premmenej ako bola zadeklarovan
                        if (parser_phase == second_phase) {
                            entry_s *item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            if (item_ptr != NULL) {
                                table_data_value dataValue;
                                dataValue.variable.is_init = true;
//                                dataValue.variable.value.t_int = "nieco"
                                if (strcmp(helper_var_type, "String")==0)
                                    dataValue.variable.type = valueType_string;
                                if (strcmp(helper_var_type, "double")==0)
                                    dataValue.variable.type = valueType_double;
                                if (strcmp(helper_var_type, "int")==0)
                                    dataValue.variable.type = valueType_int;
                                if (strcmp(helper_var_type, "boolean")==0)
                                    dataValue.variable.type = valueType_boolean;

                                tab_t_data * data = myMalloc(sizeof(tab_t_data));
                                data->type = tTypeVariable;
                                data->value = dataValue;

                                htInsert(function_symbol_table_ptr, helper_key, data);

                            }else{
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }


                        //vratime tokeny z nasho lokalneho bufferu do scannera
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            if(helperTokenBuffer[i]!=NULL){
#ifdef _DEBUG_BUFFER
                                printf("kopirujem hodnotu\n");
#endif
                            }
                            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                        }

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {
                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeFunction;
                            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                            ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                            ListOfSymbolsTables->destinationSymbolTable = function_symbol_table_ptr;

                            entry_s * item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;
                            entry_s * returned_precedence_symbol;
                            returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                            printf("priradenie po deklaracii helper key %s \n",helper_key);
#endif
                            item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                printf("generovanie instrukcie priradenia\n\n");
#endif
                                ////////////////////////
                                //generovanie instrukcie
                                ////////////////////////
                                tInstr instr;
                                instr.instType = I_ASSIGN;
                                instr.addr1 = item_ptr;
                                instr.addr2 = returned_precedence_symbol;
                                instr.addr3 = NULL;

                                item_ptr = htSearch(symbol_table,current_class_key);
                                item_ptr = htSearch(item_ptr->data->value.class.class_table,current_function_key);
                                listInsertLast(item_ptr->data->value.funkce.instrList, instr);

#ifdef _DEBUG_PARSER
                                print_elements_of_list(*item_ptr->data->value.funkce.instrList);
#endif
                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }

                        //nacitame posledny token z precedencnej analyzi
                        if(token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else {
                            result_code = SYNTAX_ERROR;
                        }
                    }
                } else{
                    result_code = SYNTAX_ERROR;
                }

            } else {
                result_code = SYNTAX_ERROR;
            }

        }else if (token->state == s_identifier){ //detekcia tokenu s_identifier -> klasifikovane pristupy, priradovanie funkcii alebo vyrazov

            ////////////////////////////////////////////////////////////////////////////////////////////////////
            ////PRIRADENIA UZ DO EXISTUJUCICH PREMMENYCH
            ////////////////////////////////////////////////////////////////////////////////////////////////////

            //docasne si zapamatame nazov identifikatora pokial by sa jednalo o klasifikovany pristup
            helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
            memcpy(helper_function_key,token->data,strlen(token->data)+1);

            helper_key = myRealloc(helper_key,strlen(token->data)+1);
            memcpy(helper_key,token->data,strlen(token->data)+1);

            t_hashtable * destination_sym_tab_for_this_variable;

            //v tuto chvilu este nevieme ci je identifikator lokalna\globalna premmena, funkcia alebo trieda
            //preto skontrolujeme ci sa nachadza aspon v jednej z troch tabuliek, pokial nie, je to semanticka chyba
            if(parser_phase == second_phase){
                bool isIdentifierExists = false;

                entry_s * item_ptr = htSearch(symbol_table, helper_function_key);
                if (item_ptr != NULL){
                    isIdentifierExists = true;
                    destination_sym_tab_for_this_variable = symbol_table;
                }

                item_ptr = htSearch(symbol_table, current_class_key);
                t_hashtable *local_symbol_table_for_this_class = item_ptr->data->value.class.class_table;
                item_ptr = htSearch(local_symbol_table_for_this_class, helper_function_key);
                if (item_ptr != NULL){
                    isIdentifierExists = true;
                    destination_sym_tab_for_this_variable = local_symbol_table_for_this_class;
                }

                item_ptr = htSearch(local_symbol_table_for_this_class, current_function_key);
                t_hashtable *local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;
                item_ptr = htSearch(local_symbol_table_for_this_function, helper_function_key);
                if (item_ptr != NULL){
                    isIdentifierExists = true;
                    destination_sym_tab_for_this_variable = local_symbol_table_for_this_function;
                }

                //identifikator sa nenasiel ani v jednej z tabuliek
                if(isIdentifierExists == false){
                    freeAndExit(SEMANTIC_ERROR);
                }

            }

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif
            //detekcia tokenu = -> bude sa jednat o priradenie funkcie ku identifikatoru
            if (token->state == s_equal_sign) {

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif

                //ulozenie tokenu pre pripadny rollback tokenov
                insertIntoHelperBuffer(token);

                if (token->state == s_identifier) { //detekcia identifikatora

                    helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
                    memcpy(helper_function_key,token->data,strlen(token->data)+1);

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif
                    //ulozenie tokenu pre pripadny rollback tokenov
                    insertIntoHelperBuffer(token);

                    //detekcia tokenu ( -> lava zatvorka
                    if (token->state == s_left_bracket) {

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        //zistime ci bola funkcia v triede zadefinovana
                        if (parser_phase == second_phase) {
                            //najskor zyskame odkaz na tabulku tabulku symbolov triedy v ktorej sa nachadzame
                            entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                            class_symbol_table_ptr = item_ptr->data->value.class.class_table;

                            //zistime ci trieda obsahuje funkciu s tymto nazvom
                            item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);
                            if (item_ptr == NULL) {
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        //vlozime novy token do lokalneho parser bufferu
                        insertIntoHelperBuffer(token);

                        result_code = parseFunctionParamList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                        //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                        token = lastToken;
                        lastToken = NULL;
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                        if (token->state != s_right_bracket) {
                            return SYNTAX_ERROR;
                        }else{
                            if(parser_phase == second_phase){
                                entry_s *item = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                                table_print(item->data->value.class.class_table);
#endif
                                item = htSearch(item->data->value.class.class_table,helper_function_key);

                                //najdem symbol ciela kam priradujem funkciu
                                entry_s * dst_item = htSearch(symbol_table, current_class_key);
                                dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_key);


                                tInstr instr;
                                instr.instType = I_CALL_FUNC;
                                instr.addr1 = dst_item; // kam funckiu priradujem
                                instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                                instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                                item = htSearch(symbol_table, current_class_key);
                                item = htSearch(item->data->value.class.class_table, current_function_key);
                                //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                                listInsertLast(item->data->value.funkce.instrList, instr);
                                function_parameter_counter = 0;
                            }
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                        if (token->state == s_semicolon) {
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        } else {
                            result_code = SYNTAX_ERROR;
                        }

                    } else { //pokial sa nejedna o lavu zatvorku bude sa jednat pravdepodobne o vyraz a riadenie prenechavame precedencnej tabulke

                        //vratime tokeny z nasho lokalneho bufferu do scannera
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                        }

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        // TODO: tu je volanie precedencnej tabulky
                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {

                            entry_s *item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            entry_s *returned_precedence_symbol;

                            if (item_ptr != NULL){
                                if (item_ptr->data->type != tTypeVariable){
                                    freeAndExit(SEMANTIC_ERROR);
                                }
                                //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                                ListOfSymbolsTables->data_type = tTypeFunction;
                                ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                                ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                                ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                                item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
    //                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                                ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;


                                returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                                printf("priradenie helper key %s \n", helper_key);
#endif
                                item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            }else{
                                item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                                if (item_ptr->data->type != tTypeVariable){
                                    freeAndExit(SEMANTIC_ERROR);
                                }
                                if(item_ptr != NULL) {
                                    //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                                    ListOfSymbolsTables->data_type = tTypeFunction;
                                    ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                                    ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                                    ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                                    item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                                    //                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                                    ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;


                                    returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                                    printf("priradenie helper key %s \n", helper_key);
#endif
                                    item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                                }else{
                                    freeAndExit(SYNTAX_ERROR);
                                }
                            }


                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.variable.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.variable.type == valueType_int)) {
                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                printf("generovanie instrukcie priradenia\n");
#endif
                                tInstr instr;
                                instr.instType = I_ASSIGN;
                                instr.addr1 = item_ptr;
                                instr.addr2 = returned_precedence_symbol;
                                instr.addr3 = NULL;

                                item_ptr = htSearch(symbol_table,current_class_key);
                                item_ptr = htSearch(item_ptr->data->value.class.class_table,current_function_key);
                                listInsertLast(item_ptr->data->value.funkce.instrList, instr);

#ifdef _DEBUG_PARSER
                                print_elements_of_list(*item_ptr->data->value.funkce.instrList);
#endif
                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }


                        //nacitame posledny token z precedencnej analyzi
                        if(token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else {
                            result_code = SYNTAX_ERROR;
                        }
                    }
                }else if(token->state == s_classified_identifier){

                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    ////KLASIFIKOVANY PRISTUP NA PRAVEJ STRANE VYRAZU DO SKOR DEKLAROVANEJ PREMENNEJ
                    ////////////////////////////////////////////////////////////////////////////////////////////////////

                    //ulozenie tokenu pre pripadny rollback tokenov
                    insertIntoHelperBuffer(token);

                    //rozparsujeme klasifikovany pristup a ulozime do pomocnej tabulky symbolov
                    classifiedIdentifier = parseClassifiedIdentifier(token, classifiedIdentifier);

                    helper_key = myRealloc(helper_key,strlen(token->data)+1);
                    memcpy(helper_key,token->data,strlen(token->data)+1);

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif
                    //odkomentovane lebo toto by mala vyriesit podstate precedencka;
//                    if(token->state == s_semicolon){
//
//                        initParserBuffer(); //vyprazdnime lokalny  buffer po tom co vieme ze ho uz nebudeme potrebovat
//
//                        //skontrolujeme ci druhy identirikator nie je nahodou funkcia
//                        if (parser_phase == second_phase) {
//                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
//                            t_hashtable * class_table_for_this_class = item->data->value.class.class_table;
//                            item = htSearch(class_table_for_this_class, classifiedIdentifier->var_identifier);
//                            if(item->data->type == tTypeFunction){
//                                freeAndExit(SEMANTIC_ERROR);
//                            }
//                        }
//
//                        //rekurzivne zavolanie parsovania tela funkcie
//                        result_code = parseFunctionBodyList();
//                        if (result_code != OK) {
//                            return SYNTAX_ERROR;
//                        }
//                    }else
                    if(token->state == s_left_bracket){

                        initParserBuffer(); //vyprazdnime lokalny  buffer po tom co vieme ze ho uz nebudeme potrebovat

                        //skontrolujeme ci druhy identirikator nie je nahodou premenna
                        if (parser_phase == second_phase) {
                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                            t_hashtable * class_symbol_table_for_this_class = item->data->value.class.class_table;
                            item = htSearch(class_symbol_table_for_this_class, classifiedIdentifier->var_identifier);
                            //fixme treba odtestovat ci prejde klasifikovane volanie funkcie aj ked druhy identifikator je variable
                            if(item->data->type == tTypeVariable){
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        result_code = parseFunctionParamList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                        //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                        token = lastToken;
                        lastToken = NULL;

#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                        if (token->state != s_right_bracket) {
                            return SYNTAX_ERROR;
                        }else{
                            ////////////////////////////////////////
                            //generovanie instrukcie skoku funkcie
                            ////////////////////////////////////////

                            if (parser_phase == second_phase) {
                                //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
                                if (classifiedIdentifier->isBuiltInFunction == false) {
                                    //TODO: vykona sa instrukcia skoku do funkcie
                                    entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
#ifdef _DEBUG_PARSER
                                    table_print(item->data->value.class.class_table);
#endif
                                    item = htSearch(item->data->value.class.class_table,classifiedIdentifier->var_identifier);

                                    //najdem symbol ciela kam priradujem funkciu
                                    entry_s * dst_item = htSearch(symbol_table, current_class_key);
                                    dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                    dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_function_key);

                                    tInstr instr;
                                    instr.instType = I_CALL_FUNC;
                                    instr.addr1 = dst_item; // kam funckiu priradujem
                                    instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                                    instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                                    item = htSearch(symbol_table, current_class_key);
                                    item = htSearch(item->data->value.class.class_table, current_function_key);
                                    //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                                    listInsertLast(item->data->value.funkce.instrList, instr);
                                    function_parameter_counter = 0;

                                } else {
//                                    table_print(class_symbol_table_ptr);
//                                    entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
                                    entry_s *item = htSearch(symbol_table, current_class_key);
                                    item = htSearch(item->data->value.class.class_table, current_function_key);

                                    //najdem symbol ciela kam priradujem funkciu
                                    entry_s * dst_item =  htSearch(symbol_table, current_class_key);
                                    dst_item = htSearch(dst_item->data->value.class.class_table, current_function_key);
                                    dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_function_key);
                                    first_builtin_operand = dst_item;

                                    //vlozime instrukciu zavolania vstavanej funkcie na instrukcnu pasku
                                    generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);

                                }
                            }
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                        if (token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        } else{
                            result_code = SYNTAX_ERROR;
                        }
                    }else{
                        //TODO: volanie precedencnej analyzi v klasifikovanom pristupe
                        //vratime tokeny z nasho lokalneho bufferu do scannera
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                        }

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        // TODO: tu je volanie precedencnej tabulky
                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {

                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);

                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeFunction;
                            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                            ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                            ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                            entry_s * returned_precedence_symbol;
                            returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                            printf("priradenie po deklaracii - na lavej strane klasifikovany pristup helper key %s \n",helper_key);
#endif
                            item_ptr = htSearch(function_symbol_table_ptr, helper_key);

                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                printf("generovanie instrukcie priradenia\n");
#endif
                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }


                        //nacitame posledny token z precedencnej analyzi
                        if(token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else {
                            result_code = SYNTAX_ERROR;
                        }
                    }
                }else {
                    //                        result_code = SYNTAX_ERROR;
                    //vratime tokeny z nasho lokalneho bufferu do scannera
                    for (int i = 0; i < BUFFER_SIZE; i++) {
                        if (helperTokenBuffer[i] != NULL) {
#ifdef _DEBUG_BUFFER
                            printf("kopirujem hodnotu\n");
#endif
                        }
                        returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                    }

                    initParserBuffer(); //reinicializaciou vynulujeme buffer

                    if (parser_phase == first_phase) {
                        precedenceCore(first_phase);
                    } else if (parser_phase == second_phase) {
                        entry_s *item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                        entry_s *returned_precedence_symbol;

                        if (item_ptr != NULL){
                            if (item_ptr->data->type != tTypeVariable){
                                freeAndExit(SEMANTIC_ERROR);
                            }
                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeFunction;
                            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                            ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                            ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                            item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                            //                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;


                            returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                            printf("priradenie helper key %s \n", helper_key);
#endif
                            item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                        }else{
                            item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                            if (item_ptr->data->type != tTypeVariable){
                                freeAndExit(SEMANTIC_ERROR);
                            }
                            if(item_ptr != NULL) {
                                //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                                ListOfSymbolsTables->data_type = tTypeFunction;
                                ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                                ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                                ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                                item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
                                //                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                                ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;


                                returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                                printf("priradenie helper key %s \n", helper_key);
#endif
                                item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                            }else{
                                freeAndExit(SYNTAX_ERROR);
                            }
                        }

                        //pred generovanim instrukcie spravime typovu kontrlou
                        if (item_ptr->data->value.variable.type ==
                            returned_precedence_symbol->data->value.constant.type ||
                            (item_ptr->data->value.variable.type == valueType_double &&
                             returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                            item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                            //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                            printf("generovanie instrukcie priradenia\n");
#endif
                            ////////////////////////
                            //generovanie instrukcie
                            ////////////////////////
                            tInstr instr;
                            instr.instType = I_ASSIGN;
                            instr.addr1 = item_ptr;
                            instr.addr2 = returned_precedence_symbol;
                            instr.addr3 = NULL;

                            item_ptr = htSearch(symbol_table,current_class_key);
                            item_ptr = htSearch(item_ptr->data->value.class.class_table,current_function_key);
                            listInsertLast(item_ptr->data->value.funkce.instrList, instr);

#ifdef _DEBUG_PARSER
                            print_elements_of_list(*item_ptr->data->value.funkce.instrList);
#endif
                        } else {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    }

                    //nacitame posledny token z precedencnej analyzi
                    if(token->state == s_semicolon){
                        //rekurzivne zavolanie parsovania tela funkcie
                        result_code = parseFunctionBodyList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                    }else {
                        result_code = SYNTAX_ERROR;
                    }

                }
            } else if (token->state == s_left_bracket) { // detekcia tokenu ( -> bude sa jednat o zavolanie funkcie bez priradenia

                //zistime ci bola funkcia v triede zadefinovana
                if (parser_phase == second_phase) {
                    //najskor zyskame odkaz na tabulku tabulku symbolov triedy v ktorej sa nachadzame
                    entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                    class_symbol_table_ptr = item_ptr->data->value.class.class_table;

                    //zistime ci trieda obsahuje funkciu s tymto nazvom
                    item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);
                    if (item_ptr == NULL) {
                        freeAndExit(SEMANTIC_ERROR);
                    }
                }

                //vlozime novy token do lokalneho parser bufferu
                insertIntoHelperBuffer(token);

                result_code = parseFunctionParamList();
                if (result_code != OK) {
                    return SYNTAX_ERROR;
                }
                //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                token = lastToken;
                lastToken = NULL;
#ifdef _DEBUG_PARSER
                fprintf(stdout, "returned token: %s\n", token->data);
#endif

                //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                if (token->state != s_right_bracket) {
                    return SYNTAX_ERROR;
                }else{
                    if(parser_phase == second_phase){
                        entry_s *item = htSearch(symbol_table, current_class_key);
#ifdef _DEBUG_PARSER
                        table_print(item->data->value.class.class_table);
#endif
                        item = htSearch(item->data->value.class.class_table,helper_function_key);

                        //najdem symbol ciela kam priradujem funkciu
                        entry_s * dst_item = NULL;

                        tInstr instr;
                        instr.instType = I_CALL_FUNC;
                        instr.addr1 = dst_item; // kam funckiu priradujem
                        instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                        instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                        item = htSearch(symbol_table, current_class_key);
                        item = htSearch(item->data->value.class.class_table, current_function_key);
                        //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                        listInsertLast(item->data->value.funkce.instrList, instr);
                        function_parameter_counter = 0;
                    }
                }

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                if (token->state == s_semicolon){
                    //rekurzivne zavolanie parsovania tela funkcie
                    result_code = parseFunctionBodyList();
                    if (result_code != OK) {
                        return SYNTAX_ERROR;
                    }
                }else{
                    result_code = SYNTAX_ERROR;
                }

            } else {
                result_code = SYNTAX_ERROR;
            }
        }else if(token->state == s_classified_identifier){

            ////////////////////////////////////////////////////////////////////////////////////////////////////
            ////KLASIFIKOVANY PRISTUP NA LAVAEJ STRANE VYRAZU
            ////////////////////////////////////////////////////////////////////////////////////////////////////

//            helper_key = myRealloc(helper_key,sizeof(token->data)+1);
            helper_key = myRealloc(helper_key,strlen(token->data)+1);
            memcpy(helper_key,token->data,strlen(token->data)+1);

            helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
            memcpy(helper_function_key,token->data,strlen(token->data)+1);


            //TODO: dorobit klasifikovany pristup
            classifiedIdentifier = parseClassifiedIdentifier(token, classifiedIdentifier);

            t_hashtable * destination_sym_tab_for_this_variable = classifiedIdentifier->classSymbolTable;

            token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
            fprintf(stdout, "current token: %s\n", token->data);
#endif


            if(token->state == s_left_bracket){
                if(classifiedIdentifier->isBuiltInFunction == false) {
                    //skontrolujeme ci druhy identirikator nie je nahodou premenna
                    if (parser_phase == second_phase) {
                        entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                        t_hashtable *class_symbol_table_for_this_class = item->data->value.class.class_table;
                        item = htSearch(class_symbol_table_for_this_class, classifiedIdentifier->var_identifier);
                        //fixme treba odtestovat ci prejde klasifikovane volanie funkcie aj ked druhy identifikator je variable
                        if (item->data->type == tTypeVariable) {
                            freeAndExit(SEMANTIC_ERROR);
                        }
                    }
                }

                result_code = parseFunctionParamList();
                if (result_code != OK) {
                    return SYNTAX_ERROR;
                }
                //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                token = lastToken;
                lastToken = NULL;

#ifdef _DEBUG_PARSER
                fprintf(stdout, "returned token: %s\n", token->data);
#endif

                //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                if (token->state != s_right_bracket) {
                    return SYNTAX_ERROR;
                }else {
                    ////////////////////////////////////////
                    //generovanie instrukcie skoku funkcie
                    ////////////////////////////////////////

//                    if (parser_phase == second_phase) {
//                        //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
//                        if (classifiedIdentifier->isBuiltInFunction == false) {
//                            //TODO: vykona sa instrukcia skoku do funkcie
//                        } else {
////                            table_print(class_symbol_table_ptr);
////                            entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
//                            entry_s *item = htSearch(symbol_table, current_class_key);
//                            item = htSearch(item->data->value.class.class_table, current_function_key);
//
//                            generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);
//                        }
//                    }
                    if (parser_phase == second_phase) {
                        //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
                        if (classifiedIdentifier->isBuiltInFunction == false) {
                            //TODO: vykona sa instrukcia skoku do funkcie
                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
#ifdef _DEBUG_PARSER
                            table_print(item->data->value.class.class_table);
#endif
                            item = htSearch(item->data->value.class.class_table,classifiedIdentifier->var_identifier);

                            //najdem symbol ciela kam priradujem funkciu
                            entry_s * dst_item = NULL;

                            tInstr instr;
                            instr.instType = I_CALL_FUNC;
                            instr.addr1 = dst_item; // kam funckiu priradujem
                            instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                            instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                            item = htSearch(symbol_table, current_class_key);
                            item = htSearch(item->data->value.class.class_table, current_function_key);
                            //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                            listInsertLast(item->data->value.funkce.instrList, instr);
                            function_parameter_counter = 0;

                        } else {
//                                    table_print(class_symbol_table_ptr);
//                                    entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
                            entry_s *item = htSearch(symbol_table, current_class_key);
                            item = htSearch(item->data->value.class.class_table, current_function_key);

                            //najdem symbol ciela kam priradujem funkciu
                            first_builtin_operand = NULL;

                            //vlozime instrukciu zavolania vstavanej funkcie na instrukcnu pasku
                            generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);

                        }
                    }
                }

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                if (token->state == s_semicolon){
                    //rekurzivne zavolanie parsovania tela funkcie
                    result_code = parseFunctionBodyList();
                    if (result_code != OK) {
                        return SYNTAX_ERROR;
                    }
                } else{
                    result_code = SYNTAX_ERROR;
                }

            }else if (token->state == s_equal_sign){

                token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                fprintf(stdout, "current token: %s\n", token->data);
#endif
                //ulozenie tokenu pre pripadny rollback tokenov
                insertIntoHelperBuffer(token);

                if (token->state == s_identifier) { //detekcia identifikatora
                    helper_function_key = myRealloc(helper_function_key,strlen(token->data)+1);
                    memcpy(helper_function_key,token->data,strlen(token->data)+1);

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif
                    //detekcia tokenu ( -> lava zatvorka
                    if (token->state == s_left_bracket) {

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        //zistime ci bola funkcia v triede zadefinovana
                        if (parser_phase == second_phase) {
                            //najskor zyskame odkaz na tabulku tabulku symbolov triedy v ktorej sa nachadzame
                            entry_s *item_ptr = htSearch(symbol_table, current_class_key);
                            class_symbol_table_ptr = item_ptr->data->value.class.class_table;

                            //zistime ci trieda obsahuje funkciu s tymto nazvom
                            item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);
                            if (item_ptr == NULL) {
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        //vlozime novy token do lokalneho parser bufferu
                        insertIntoHelperBuffer(token);

                        result_code = parseFunctionParamList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                        //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                        token = lastToken;
                        lastToken = NULL;
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                        if (token->state != s_right_bracket) {
                            return SYNTAX_ERROR;
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                        if (token->state == s_semicolon) {
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        } else {
                            result_code = SYNTAX_ERROR;
                        }

                    }else{
                        //vratime tokeny z nasho lokalneho bufferu do scannera
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                        }
                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        // TODO: tu je volanie precedencnej tabulky
                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {
                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeFunction;
                            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                            ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                            ListOfSymbolsTables->destinationSymbolTable = classifiedIdentifier->classSymbolTable;

                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                            entry_s * returned_precedence_symbol;
                            returned_precedence_symbol = precedenceCore(second_phase);

#ifdef _DEBUG_PARSER
                            printf("priradenie helper key %s \n",helper_key);
#endif
                            item_ptr = htSearch(classifiedIdentifier->classSymbolTable, helper_key);

                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                printf("generovanie instrukcie priradenia do klasifikovaneho pristupu\n");
#endif
                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }


                        //nacitame posledny token z precedencnej analyzi
                        if(token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else {
                            result_code = SYNTAX_ERROR;
                        }

                    }
                }else if(token->state == s_classified_identifier){
                    ////////////////////////////////////////////////////////////////////////////////////////////////////
                    ////KLASIFIKOVANY PRISTUP NA PRAVEJ STRANE VYRAZU - PRIRADUJEME DO KLASIFIKOVANEHO VYRAZU NA LAVEJ STRANE VYRAZU
                    ////////////////////////////////////////////////////////////////////////////////////////////////////

                    //rozparsujeme klasifikovany pristup a ulozime do pomocnej tabulky symbolov
                    classifiedIdentifier = parseClassifiedIdentifier(token, classifiedIdentifier);

                    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                    fprintf(stdout, "current token: %s\n", token->data);
#endif
                    //ulozenie dalsieho tokenu do bufferu pre pripad rollback tokenov
                    insertIntoHelperBuffer(token);

                    if(token->state == s_left_bracket){

                        initParserBuffer(); //vyprazdnime lokalny  buffer po tom co vieme ze ho uz nebudeme potrebovat

                        //skontrolujeme ci druhy identirikator nie je nahodou premenna
                        if (parser_phase == second_phase) {
                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                            t_hashtable * class_symbol_table_for_this_class = item->data->value.class.class_table;
                            item = htSearch(class_symbol_table_for_this_class, classifiedIdentifier->var_identifier);
                            //fixme treba odtestovat ci prejde klasifikovane volanie funkcie aj ked druhy identifikator je variable
                            if(item->data->type == tTypeVariable){
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }

                        result_code = parseFunctionParamList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                        //ziskame naspat token z urovne nizssie z funkcie parseFunctionParamList()
                        token = lastToken;
                        lastToken = NULL;

#ifdef _DEBUG_PARSER
                        fprintf(stdout, "returned token: %s\n", token->data);
#endif

                        //detekcia ci ziskany(navrateny) token je ) -> ukoncenie deklaracie argumentov funkcie
                        if (token->state != s_right_bracket) {
                            return SYNTAX_ERROR;
                        }else{
                            ////////////////////////////////////////
                            //generovanie instrukcie skoku funkcie
                            ////////////////////////////////////////

                            if(parser_phase == second_phase){
                                //TODO: generovanie instrukcie skokudo funkcie - pripadne vykonanie vstavanej funkcie
                                if (classifiedIdentifier->isBuiltInFunction == false) {
                                    //TODO: vykona sa instrukcia skoku do funkcie
                                    entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
#ifdef _DEBUG_PARSER
                                    table_print(item->data->value.class.class_table);
#endif
                                    item = htSearch(item->data->value.class.class_table,classifiedIdentifier->var_identifier);

                                    //najdem symbol ciela kam priradujem funkciu
                                    entry_s * dst_item = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                                    dst_item = htSearch(dst_item->data->value.class.class_table, classifiedIdentifier->var_identifier);
                                    dst_item = htSearch(dst_item->data->value.funkce.local_t, helper_function_key);

                                    tInstr instr;
                                    instr.instType = I_CALL_FUNC;
                                    instr.addr1 = dst_item; // kam funckiu priradujem
                                    instr.addr2 = item;     // odkaz na funkciu do tabulky symbolov
                                    instr.addr3 = functionHelperParameterInstructionList; //pomocna instrukcna paska parametrov pre funkciu

                                    item = htSearch(symbol_table, current_class_key);
                                    item = htSearch(item->data->value.class.class_table, current_function_key);
                                    //vlozime instrukciu zavolania funkcie na instrukcnu pasku
                                    listInsertLast(item->data->value.funkce.instrList, instr);
                                    function_parameter_counter = 0;

                                } else {
//                                    table_print(class_symbol_table_ptr);
//                                    entry_s *item = htSearch(class_symbol_table_ptr, current_function_key);
                                    entry_s *item = htSearch(symbol_table, current_class_key);
                                    item = htSearch(item->data->value.class.class_table, current_function_key);

                                    //najdem symbol ciela kam priradujem funkciu
                                    char *str_helper = myMalloc(strlen(helper_function_key)+1);
                                    str_helper = memcpy(str_helper, helper_function_key, strlen(helper_function_key)+1);

                                    char * class = strsep(&str_helper, ".");
                                    char * var = strsep(&str_helper, ".");

                                    entry_s * dst_item = htSearch(symbol_table, class);
                                    dst_item = htSearch(dst_item->data->value.class.class_table, var);
                                    first_builtin_operand = dst_item;

                                    //vlozime instrukciu zavolania vstavanej funkcie na instrukcnu pasku
                                    generateInstructionForBuiltInFunction(item->data->value.funkce.instrList);

                                }
                            }
                        }

                        token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
                        fprintf(stdout, "current token: %s\n", token->data);
#endif
                        //detekcia tokenu ; -> za volanim funkcie musi nasledovat bodkociarka
                        if (token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        } else{
                            result_code = SYNTAX_ERROR;
                        }
                    }else{
                        //TODO: volanie precedencnej analyzi v klasifikovanom pristupe
                        //vratime tokeny z nasho lokalneho bufferu do scannera
                        for (int i = 0; i < BUFFER_SIZE; i++) {
                            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                        }

                        initParserBuffer(); //reinicializaciou vynulujeme buffer

                        // TODO: tu je volanie precedencnej tabulky
                        if(parser_phase == first_phase) {
                            precedenceCore(first_phase);
                        } else if (parser_phase == second_phase) {

                            entry_s *item = htSearch(symbol_table, classifiedIdentifier->class_identifier);

                            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                            ListOfSymbolsTables->data_type = tTypeFunction;
                            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                            ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                            ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                            entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                            entry_s * returned_precedence_symbol;
                            returned_precedence_symbol = precedenceCore(second_phase);

                            classifiedIdentifier->class_identifier = strsep(&helper_key, ".");
                            classifiedIdentifier->var_identifier = strsep(&helper_key, ".");

#ifdef _DEBUG_PARSER
                            printf("priradenie po deklaracii - na lavej strane klasifikovany pristup helper key %s \n",classifiedIdentifier->var_identifier);
#endif
                            item_ptr = htSearch(destination_sym_tab_for_this_variable, classifiedIdentifier->var_identifier);

                            //pred generovanim instrukcie spravime typovu kontrlou
                            if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                               (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                                item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                                //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                                printf("generovanie instrukcie priradenia\n");
#endif
                            }else{
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }


                        //nacitame posledny token z precedencnej analyzi
                        if(token->state == s_semicolon){
                            //rekurzivne zavolanie parsovania tela funkcie
                            result_code = parseFunctionBodyList();
                            if (result_code != OK) {
                                return SYNTAX_ERROR;
                            }
                        }else {
                            result_code = SYNTAX_ERROR;
                        }
                    }

                }else{
//                    result_code = SYNTAX_ERROR;
                    //vratime tokeny z nasho lokalneho bufferu do scannera
                    for (int i = 0; i < BUFFER_SIZE; i++) {
                        returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
                    }
                    initParserBuffer(); //reinicializaciou vynulujeme buffer

                    // TODO: tu je volanie precedencnej tabulky
                    if(parser_phase == first_phase) {
                        precedenceCore(first_phase);
                    } else if (parser_phase == second_phase) {
                        //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
                        ListOfSymbolsTables->data_type = tTypeFunction;
                        ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
                        ListOfSymbolsTables->functionSymbolTable = function_symbol_table_ptr;
                        ListOfSymbolsTables->destinationSymbolTable = destination_sym_tab_for_this_variable;

                        entry_s * item_ptr = htSearch(function_symbol_table_ptr, helper_key);
                        ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList;

                        entry_s * returned_precedence_symbol;
                        returned_precedence_symbol = precedenceCore(second_phase);

                        classifiedIdentifier->class_identifier = strsep(&helper_key, ".");
                        classifiedIdentifier->var_identifier = strsep(&helper_key, ".");

#ifdef _DEBUG_PARSER
                        printf("priradenie helper key %s \n",classifiedIdentifier->var_identifier);
#endif
                        item_ptr = htSearch(destination_sym_tab_for_this_variable, classifiedIdentifier->var_identifier);

                        //pred generovanim instrukcie spravime typovu kontrlou
                        if(item_ptr->data->value.variable.type == returned_precedence_symbol->data->value.constant.type ||
                           (item_ptr->data->value.variable.type == valueType_double && returned_precedence_symbol->data->value.constant.type == valueType_int)) {
                            item_ptr->data->value.variable.is_init = true; // do tabulky symbolov zaznamename ze premmena bola inicializovana
                            //TODO generovat instrukciu priradenia return_precedence_symbol do item_ptr
#ifdef _DEBUG_PARSER
                            printf("generovanie instrukcie priradenia do klasifikovaneho pristupu\n");
#endif
                        }else{
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    }


                    //nacitame posledny token z precedencnej analyzi
                    if(token->state == s_semicolon){
                        //rekurzivne zavolanie parsovania tela funkcie
                        result_code = parseFunctionBodyList();
                        if (result_code != OK) {
                            return SYNTAX_ERROR;
                        }
                    }else {
                        result_code = SYNTAX_ERROR;
                    }
                }

            }else{
                result_code = SYNTAX_ERROR;
            }

        } else{

            //ziaden dalsi token v tele funkcie byt nemoze
            result_code = SYNTAX_ERROR;
        }
    }

    return result_code;
}


/**
 * parsovanie parametrov funkcie je jej zavolani
 */
int parseFunctionParamList(){
    int result_code = OK;


    initParserBuffer();
    token = getNextTokenFromScanner();
#ifdef _DEBUG_PARSER
    fprintf(stdout, "current token: %s\n", token->data);
#endif
    insertIntoHelperBuffer(token);

    //detekcia tokenu ) -> pouzite epsilon pravidlo, funkcia je bez deklarovanych argumentov
    if (token->state == s_right_bracket) {
        initParserBuffer();
        //pri zisteny pravej zatvorky sa moze jednat o prazdy prikaz alebo ukoncenie deklaracii argumentov,
        //z predoslej rekurzie tam vsak mozeme mat ciarku za ktorou nemoze nasledovat prava zatvorka preto nastavime result_code na SYNTAX_ERROR
        if(lastToken != NULL){ //napr. last_token->state == s_comma
            lastToken = NULL;
            result_code = SYNTAX_ERROR;
        } else {
            lastToken = token;
            result_code = OK;
        }

        if(parser_phase == second_phase) {
            const char ch = '.';
            char *ret = strchr(helper_key, ch);

            entry_s *item_ptr;

            //identifikujeme ci sa jedna o klasifikovany pristup
            if (ret == NULL) {
                item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);

                //kontrolujeme ci sedi pocet parametrov
                if (parser_phase == second_phase) {

                    if (function_parameter_counter < item_ptr->data->value.funkce.argcount) {
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }


                //nastavime ze sa nejedna o vstavanu funkciu lebo dalej nam to robilo sarapatu
                classifiedIdentifier->isBuiltInFunction = false;
            } else {

                item_ptr = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                t_hashtable *local_symbol_table_for_this_class = item_ptr->data->value.class.class_table;

                item_ptr = htSearch(local_symbol_table_for_this_class, classifiedIdentifier->var_identifier);

                if (parser_phase == second_phase) {
                    if (function_parameter_counter < item_ptr->data->value.funkce.argcount) {
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }

            }
        }


    }else{
        //pri rekurzii sa mohol lastToken nastavit na nejaku hodnotu
        //nastavime ho zasa na NULL nech vieme ze nepotrebujeme vracat ziaden token na uroven vyssie
        if (lastToken != NULL){
            lastToken = NULL;
        }

        //vratime tokeny z nasho lokalneho bufferu do scannera
        for (int i = 0; i < BUFFER_SIZE; i++) {
            returnTokenAndSaveToBuffer(helperTokenBuffer[i]);
        }
        initParserBuffer(); //reinicializaciou vynulujeme buffer

//        printf("current function key: %s\n",current_function_key);
//        entry_s * item_ptr = htSearch(class_symbol_table_ptr, current_function_key);
//        t_hashtable *local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;


        if(parser_phase == first_phase) {
            precedenceCore(first_phase);
        } else if (parser_phase == second_phase) {
            const char ch = '.';
            char *ret = strchr(helper_key, ch);

            entry_s *item_ptr;
            t_hashtable *local_symbol_table_for_this_function;


            //identifikujeme ci sa jedna o klasifikovany pristup
            if (ret == NULL) {
//                item_ptr = htSearch(class_symbol_table_ptr, helper_key);
                item_ptr = htSearch(class_symbol_table_ptr, helper_function_key);

                //kontrolujeme ci sedi pocet parametrov
                if (parser_phase == second_phase) {
#ifdef _DEBUG_PARSER
                    printf("suck shit: %d \n",function_parameter_counter);
                    printf("suck shit: %d \n",item_ptr->data->value.funkce.argcount);
#endif
                    function_parameter_counter = function_parameter_counter + 1;
                    if (function_parameter_counter > item_ptr->data->value.funkce.argcount) {
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }

                local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;

                //nastavime ze sa nejedna o vstavanu funkciu lebo dalej nam to robilo sarapatu
                classifiedIdentifier->isBuiltInFunction = false;
            } else {

//                if (classifiedIdentifier->isBuiltInFunction == false) {
                    item_ptr = htSearch(symbol_table, classifiedIdentifier->class_identifier);
                    t_hashtable *local_symbol_table_for_this_class = item_ptr->data->value.class.class_table;

                    item_ptr = htSearch(local_symbol_table_for_this_class, classifiedIdentifier->var_identifier);
                    local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;
//                } else {
//
//                    item_ptr = htSearch(symbol_table, current_class_key);
//                    t_hashtable *local_symbol_table_for_this_class = item_ptr->data->value.class.class_table;
//
//                    item_ptr = htSearch(local_symbol_table_for_this_class, current_function_key);
//                    local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;
//
//                }

                if (parser_phase == second_phase) {
#ifdef _DEBUG_PARSER
                    printf("suck shit: %d \n",function_parameter_counter);
                    printf("suck shit: %d \n",item_ptr->data->value.funkce.argcount);
                    printf("suck shit: %s \n",item_ptr->key);
#endif
                    function_parameter_counter = function_parameter_counter + 1;
                    if (function_parameter_counter > item_ptr->data->value.funkce.argcount) {
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }
                }



//                item_ptr = htSearch(symbol_table, current_class_key);
//                //kontrolujeme ci sedi pocet parametrov
//                if(parser_phase == second_phase) {
//                    if (function_parameter_counter != item_ptr->data->value.funkce.argcount) {
//                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                    }
//                    function_parameter_counter = function_parameter_counter + 1;
//                }
//                t_hashtable *local_symbol_table_for_this_class = item_ptr->data->value.class.class_table;
//
//                item_ptr = htSearch(local_symbol_table_for_this_class, current_function_key);
//                local_symbol_table_for_this_function = item_ptr->data->value.funkce.local_t;
            }

            //nasledujuce 3 riadky sluzia na to aby sme vytiahli spravnu instrukcnu paasku pre precedencku
            entry_s * item_helper = htSearch(symbol_table, current_class_key);
            t_hashtable *symbol_table_for_current_class = item_helper->data->value.class.class_table;
            item_helper = htSearch(symbol_table_for_current_class, current_function_key);

            //nastavime precedencnej strukturu s odkazmi na tabulky symbolov kde bude prehladavat
            ListOfSymbolsTables->data_type = tTypeFunction;
//            ListOfSymbolsTables->classSymbolTable = class_symbol_table_ptr;
            ListOfSymbolsTables->classSymbolTable = symbol_table_for_current_class;
//            ListOfSymbolsTables->functionSymbolTable = local_symbol_table_for_this_function;
            ListOfSymbolsTables->functionSymbolTable = item_helper->data->value.funkce.local_t;
            ListOfSymbolsTables->destinationSymbolTable = local_symbol_table_for_this_function;
            ListOfSymbolsTables->instList = item_ptr->data->value.funkce.instrList; //todo: mozno neposielam zakazdym spravnu tabulku symbolov

            entry_s *returned_precedence_symbol;
            returned_precedence_symbol = precedenceCore(second_phase);


            //TODO: spravit typovu kontrolu a vygenerovat instrukciu

            if (parser_phase == second_phase) {
                //najskor sa pozreme ci sa nejedna o vstavanu funkciu;
                if (classifiedIdentifier->isBuiltInFunction == true) {
                    item_ptr = htSearch(symbol_table, "ifj16");

                    if (strcmp(classifiedIdentifier->var_identifier, "print") == 0) {
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "print");

                        item_ptr = htSearch(item_ptr->data->value.funkce.local_t,
                                            "term"); //term je nazov parametru pre print

                        //rebehne typova kontrola
                        if (item_ptr->data->value.variable.type == valueType_double ||
                            item_ptr->data->value.variable.type == valueType_int ||
                            item_ptr->data->value.variable.type == valueType_string) {
                            //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                            if (function_parameter_counter == 1) {
                                second_builtin_operand = returned_precedence_symbol;
                            } else if (function_parameter_counter == 2) {
                                third_builtin_operand = NULL;
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }

                        } else {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }

                    } else if (strcmp(classifiedIdentifier->var_identifier, "readIn") == 0) {
                        //readIn neobsahuje parametre
                        if (function_parameter_counter-1 > 0) {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    } else if (strcmp(classifiedIdentifier->var_identifier, "readDouble") == 0) {
                        //readIn neobsahuje parametre
                        if (function_parameter_counter-1 > 0) {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    } else if (strcmp(classifiedIdentifier->var_identifier, "readString") == 0) {
                        //readIn neobsahuje parametre
                        if (function_parameter_counter-1 > 0) {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    } else if (strcmp(classifiedIdentifier->var_identifier, "length") == 0) {
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "length");

                        item_ptr = htSearch(item_ptr->data->value.funkce.local_t,
                                            "term"); //term je nazov parametru pre print

                        //rebehne typova kontrola
                        if (item_ptr->data->value.variable.type == valueType_string) {
                            //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                            if (function_parameter_counter == 1) {
                                second_builtin_operand = returned_precedence_symbol;
                            } else if (function_parameter_counter == 2) {
                                third_builtin_operand = NULL;
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }

                        } else {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    }else if (strcmp(classifiedIdentifier->var_identifier, "sort") == 0) {
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "sort");

                        item_ptr = htSearch(item_ptr->data->value.funkce.local_t,
                                            "term"); //term je nazov parametru pre print

                        //rebehne typova kontrola
                        if (item_ptr->data->value.variable.type == valueType_string) {
                            //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                            if (function_parameter_counter == 1) {
                                second_builtin_operand = returned_precedence_symbol;
                            } else if (function_parameter_counter == 2) {
                                third_builtin_operand = NULL;
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }

                        } else {
                            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                        }
                    }else if (strcmp(classifiedIdentifier->var_identifier, "substr") == 0) {
                        //todo substr sa bude cele prerabat a robit inak ako ostatne
//                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "substr");
//
//                        //prebehne typova kontrola
//                        t_hashtable *helper_table = item_ptr->data->value.funkce.local_t;
//                        if (function_parameter_counter == 1) {
//                            item_ptr = htSearch(helper_table, "term1"); //term je nazov parametru pre print
//                            if (item_ptr->data->value.variable.type == valueType_string) {
//                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
//                                if (function_parameter_counter == 1) {
//                                    first_builtin_operand = returned_precedence_symbol;
//                                } else {
//                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                                }
//                            } else {
//                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                            }
//                        }
//                        if (function_parameter_counter == 2) {
//                            item_ptr = htSearch(helper_table, "term2"); //term je nazov parametru pre print
//                            if (item_ptr->data->value.variable.type == valueType_int) {
//                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
//                                if (function_parameter_counter == 2) {
//                                    second_builtin_operand = returned_precedence_symbol;
//                                } else {
//                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                                }
//                            } else {
//                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                            }
//                        }
//                        if (function_parameter_counter == 3) {
//                            item_ptr = htSearch(helper_table, "term3"); //term je nazov parametru pre print
//                            if (item_ptr->data->value.variable.type == valueType_int) {
//                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
//                                if (function_parameter_counter == 3) {
//                                    third_builtin_operand = returned_precedence_symbol;
//                                } else {
//                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                                }
//                            } else {
//                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
//                            }
//                        }
                    }else if (strcmp(classifiedIdentifier->var_identifier, "compare") == 0) {
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "compare");

                        //prebehne typova kontrola
                        t_hashtable *helper_table = item_ptr->data->value.funkce.local_t;
                        if (function_parameter_counter == 1) {
                            item_ptr = htSearch(helper_table, "term1"); //term je nazov parametru pre print
                            if (item_ptr->data->value.variable.type == valueType_string) {
                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                                if (function_parameter_counter == 1) {
                                    second_builtin_operand = returned_precedence_symbol;
                                } else {
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            } else {
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }
                        if (function_parameter_counter == 2) {
                            item_ptr = htSearch(helper_table, "term2"); //term je nazov parametru pre print
                            if (item_ptr->data->value.variable.type == valueType_string) {
                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                                if (function_parameter_counter == 2) {
                                    third_builtin_operand = returned_precedence_symbol;
                                } else {
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            } else {
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }
                    }else if (strcmp(classifiedIdentifier->var_identifier, "find") == 0) {
                        item_ptr = htSearch(item_ptr->data->value.class.class_table, "find");

                        //prebehne typova kontrola
                        t_hashtable *helper_table = item_ptr->data->value.funkce.local_t;
                        if (function_parameter_counter == 1) {
                            item_ptr = htSearch(helper_table, "term1"); //term je nazov parametru pre print
                            if (item_ptr->data->value.variable.type == valueType_string) {
                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                                if (function_parameter_counter == 1) {
                                    second_builtin_operand = returned_precedence_symbol;
                                } else {
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            } else {
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }
                        if (function_parameter_counter == 2) {
                            item_ptr = htSearch(helper_table, "term2"); //term je nazov parametru pre print
                            if (item_ptr->data->value.variable.type == valueType_string) {
                                //nastavime globalne parametre pre vstavane funkcie, pri vstavanych funkciach neprebehne instrukcia set
                                if (function_parameter_counter == 2) {
                                    third_builtin_operand = returned_precedence_symbol;
                                } else {
                                    freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                                }
                            } else {
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }
                    }

                } else {
                    ////////////////////////////////////////////////////////
                    //jedna sa o beznu funkciu (nie builtin)
                    ////////////////////////////////////////////////////////
                    if (function_parameter_counter-1 > item_ptr->data->value.funkce.argcount) {
                        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                    }

                    entry_s *tmp = item_ptr->data->value.funkce.first_par;
                    for (int i = 0; i < function_parameter_counter-1; i++) {
                        tmp = tmp->next;
                    }

                    if (function_parameter_counter-1 == 0) {
                        functionHelperParameterInstructionList = myMalloc(sizeof(tListOfInstr));
                        listInit(functionHelperParameterInstructionList);
                    }

                    //typova kontrola parametru
                    if(returned_precedence_symbol != NULL) { // zistime ci funkcia mala parametre, ak precedencka vratila NULL tak nie
                        if (tmp->data->value.variable.type == returned_precedence_symbol->data->value.variable.type) {
                            //skontrolujeme ci sa jednalo o rovnaky typ dat
                            generateInstructionToHelperParamList(tmp, returned_precedence_symbol);
                        } else {
                            if (tmp->data->value.variable.type == valueType_double &&
                                    returned_precedence_symbol->data->value.variable.type == valueType_int) { //returned_precedence_symbol->data->value.variable.type
                                // skontrolujeme ci sa nejednalo o priradenie integeru do double co nie je chyba
                                generateInstructionToHelperParamList(tmp,returned_precedence_symbol);
                            } else {
                                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
                            }
                        }
                    }else{
                        //pokial je pocet parametrov vo funkcii 0 tak odkaz na pomocnu instrukcnu pasku pre parametre bude NULL
                        functionHelperParameterInstructionList = NULL;
                    }

                }

            }
        }



        //pytam sa na posledny token z precedencnej analyzi
        if (token->state == s_right_bracket) { //detekcia tokenu ) -> koniec deklaracie argumentov
            lastToken = token;
            result_code = OK;
        }else if (token->state == s_comma) { //detekcia tokenu , -> bude nasledovat deklaracia dalieho argumentu
            lastToken = token;
            result_code = parseFunctionParamList(); //rekurzivne zavolanie parsovania dalsich deklaracii argumentov
            if (result_code != OK){
                return SYNTAX_ERROR;
            }
        } else {
            result_code = SYNTAX_ERROR;
        }
    }

    return result_code;
}

/**
 * vygeneruje instrukciu SET pre pomocnu instrukcnu pasku funkcie (instrukcna paska pre parametre)
 */
void generateInstructionToHelperParamList(entry_s * dst, entry_s * src){


    // mame tam ale problem s predavanim tejto pasky takze treba vymysliet sposob ako to spravit
    //tListOfInstr * helperInstrList;
    //helperInstrList = myMalloc(sizeof(tListOfInstr));

    tInstr inst;
    inst.instType = I_SET_PARAM;
    inst.addr1 = dst;
    inst.addr2 = src;
    inst.addr3 = NULL;
    listInsertLast(functionHelperParameterInstructionList, inst);

//    functionHelperParameterInstructionList = helperInstrList;
}




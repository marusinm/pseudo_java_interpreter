/* *************************** precedence.c ********************************* */
/* Soubor:              precedence.c                                          */
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

#include "precedence.h"
#include "parser.h"
#include "stack.h"
#include "scanner.h"
#include "ial.h"
#include "common.h"

extern tListOfInstr *instList;
extern t_hashtable *symbol_table;
extern tListOfSymbolsTables *ListOfSymbolsTables;
extern tClassifiedIdentifier * classifiedIdentifier;
extern tToken *token;

int phase = 1;

static short precedence_table[PRECEDENCE_TABLE_SIZE][PRECEDENCE_TABLE_SIZE];

/*
 * Define rules for generating 3AC
 * written from right to left
 */
int pole[NUMBER_OF_RULES][3] = {
    /* E -> E + E   No. 0 */ {s_expression, s_plus, s_expression},              //{15,  3, 15}
    /* E -> E - E   No. 1 */ {s_expression, s_minus, s_expression},             //{15,  4, 15}
    /* E -> E * E   No. 2 */ {s_expression, s_multiply, s_expression},          //{15,  5, 15}
    /* E -> E / E   No. 3 */ {s_expression, s_divide, s_expression},            //{15,  6, 15}
    /* E -> E < E   No. 4 */ {s_expression, s_less, s_expression},              //{15,  7, 15}
    /* E -> E > E   No. 5 */ {s_expression, s_more, s_expression},              //{15,  8, 15}
    /* E -> E <= E  No. 6 */ {s_expression, s_less_equal, s_expression},        //{15,  9, 15}
    /* E -> E >= E  No. 7 */ {s_expression, s_more_equal, s_expression},        //{15, 10, 15}
    /* E -> E == E  No. 8 */ {s_expression, s_equal_expression, s_expression},  //{15, 11, 15}
    /* E -> E != E  No. 9 */ {s_expression, s_diff_expression, s_expression},   //{15, 12, 15}
    /* E -> (E)     No.10 */ {s_right_bracket, s_expression, s_left_bracket},   //{ 2, 15,  1}
    /* E -> i       No.11 */ {s_identifier, -1, -1},                            //{13, -1, -1}
    /* E -> !E      No.12 */ {s_expression, s_binary_not, -1},                  //{15, 18, -1}
    /* E -> E || E  No.13 */ {s_expression, s_binary_or, s_expression},         //{15, 17, 15}
    /* E -> E && E  No.14 */ {s_expression, s_binary_and, s_expression},        //{15, 16, 15}
};

/*
 * TODO:
 * - pravdepodobne si prepisujem symboly klucom internal_constant a retype_constant,chcelo by to funkciu na generovanie klucov
 * - allocovanie pamate pre stack, zatial tam je defaultne 200
 * - check if constant exist in symbol table
 *****************************************************************************************************
 * Relační operátory nepodporují porovnání řetězců
 *****************************************************************************************************
 * Je-li to nutné, bude interpret provádět implicitní konverze operandů i výsledků výrazů z int na double.
 *****************************************************************************************************
 * Jiné než uvedené kombinace typů (včetně případných povolených implicitních konverzí)
 * ve výrazech pro popsané operátory jsou považovány za chybu 4.
 */

// check types over operations
int instAddTypes(data_value_type operand1, data_value_type operand2, instructions instType) {

    /*
     * COMPARISONS OPERATIONS
     * operands can't be string and destination value type will be BOOLEAN
     *
     */
    if (instType == I_LESS          ||
        instType == I_MORE          ||
        instType == I_LESS_EQUAL    ||
        instType == I_MORE_EQUAL    ||
        instType == I_EQUAL         ||
        instType == I_NOT_EQUAL     ){

        // nesmie byt string
        if (operand1==valueType_string || operand2==valueType_string)
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);

        return valueType_boolean;
    }

    /*
     * MATHEMATICS OPERATIONS
     * operands can't be string
     * destination value type is based on operands
     */
    if (instType == I_SUB ||
        instType == I_MUL ||
        instType == I_DIV ){

        if (operand1==valueType_string || operand2==valueType_string)
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);

    }

    /*
     * BOOLEAN OPERATIONS
     * operands can be only type of boolean
     */
    if (instType == I_OR || instType == I_AND){
        if (operand1 == valueType_boolean && operand2 == valueType_boolean)
            return valueType_boolean;
        else
            freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
    }

    if (operand1==operand2){

        switch (operand1){
            case valueType_int:
                return valueType_int;
            case valueType_double:
                return valueType_double;
            case valueType_string:
                return valueType_string;
            default:
                return -1; //error
        }
    }

    if (operand1==valueType_int && operand2==valueType_double)
        return valueType_double;

    if (operand1==valueType_double && operand2==valueType_int)
        return valueType_double;

    if (operand1==valueType_string || operand2==valueType_string)
        return valueType_string;

    return -1;
}

entry_s *intToDouble(entry_s *symbol) {

    // read value from symbol and convert it to double
    int value = symbol->data->value.constant.value.t_int;
    double new_value = (double) value;

    // generate new symbol with the original value (converted) and new type
    table_data_value constant;
    constant.constant.type = valueType_double;
    constant.constant.value.t_double = new_value;

    tab_t_data *tab_data = myMalloc(sizeof(tab_t_data));
    tab_data->type = tTypeConstant;
    tab_data->value = constant;

    // insert new symbol to symbol table
//    t_hashtable *hash_table = NULL;
//    if (ListOfSymbolsTables->data_type == tTypeFunction)
//        hash_table = ListOfSymbolsTables->functionSymbolTable;
//    else if (ListOfSymbolsTables->data_type == tTypeClass)
//        hash_table = ListOfSymbolsTables->classSymbolTable;
//    else
//        hash_table = symbol_table;

    char * key = "retype_constant";
    htInsert(ListOfSymbolsTables->destinationSymbolTable, key, tab_data);

    entry_s *item = htSearch(ListOfSymbolsTables->destinationSymbolTable, key);

#ifdef _DEBUG_PRECEDENCE
//    table_print(hashtable);
#endif


    return item;
}

data_value_type stateToValueType(tState state){
    switch (state){
        case s_int:
            return valueType_int;
        case s_double:
            return valueType_double;
        case s_string:
            return valueType_string;
        case s_boolean:
            return valueType_boolean;
        default:
            return valueType_int; //todo pouzivane pri generovani konstant v prvej fazi, overit ci to je dobre
//            break;
    }
}

void printPrecedenceTable( short (*precedence_table)[PRECEDENCE_TABLE_SIZE]){

    printf("\n PRECEDENCE TABLE \n");
    printf("################################################################\n");
    for(int i = 0; i < PRECEDENCE_TABLE_SIZE; i++) {
        customPrintState( i );
        printf("   ");
        for(int j = 0; j < PRECEDENCE_TABLE_SIZE; j++) {

            switch ( precedence_table[i][j] ){
                case precedence_less:
                    printf(" < ");
                    break;
                case precedence_equal:
                    printf(" = ");
                    break;
                case precedence_not_defined:
                    printf("err");
                    break;
                case precedence_more:
                    printf(" > ");
//                    printf("   ");
                    break;
                default:
                    break;
            }
        }
        printf("\n");
    }
    printf("################################################################\n\n");
}

void precedence_init( short (*precedence_table)[PRECEDENCE_TABLE_SIZE] ) {

    /*
     * PRECEDENCE_MORE is defined by static (zero)
     */
    precedence_table[OP_LBRACE][OP_LBRACE]      = precedence_less;
    precedence_table[OP_LBRACE][OP_RBRACE]      = precedence_equal;
    precedence_table[OP_LBRACE][OP_ADD]         = precedence_less;
    precedence_table[OP_LBRACE][OP_SUB]         = precedence_less;
    precedence_table[OP_LBRACE][OP_MUL]         = precedence_less;
    precedence_table[OP_LBRACE][OP_DIV]         = precedence_less;
    precedence_table[OP_LBRACE][OP_LESS]        = precedence_less;
    precedence_table[OP_LBRACE][OP_MORE]        = precedence_less;
    precedence_table[OP_LBRACE][OP_LESSEQUAL]   = precedence_less;
    precedence_table[OP_LBRACE][OP_MOREEQUAL]   = precedence_less;
    precedence_table[OP_LBRACE][OP_EQUAL]       = precedence_less;
    precedence_table[OP_LBRACE][OP_NOTEQUAL]    = precedence_less;
    precedence_table[OP_LBRACE][IDENTIFIER]     = precedence_less;
    precedence_table[OP_LBRACE][LOGICAL_NOT]    = precedence_less;
    precedence_table[OP_LBRACE][CONDITIONAL_OR] = precedence_less;
    precedence_table[OP_LBRACE][CONDITIONAL_AND]= precedence_less;
    precedence_table[OP_LBRACE][DOLLAR]         = precedence_not_defined;

    precedence_table[OP_RBRACE][OP_LBRACE]      = precedence_not_defined;
    precedence_table[OP_RBRACE][IDENTIFIER]     = precedence_not_defined;

    precedence_table[OP_ADD][OP_LBRACE]         = precedence_less;
    precedence_table[OP_ADD][OP_MUL]            = precedence_less;
    precedence_table[OP_ADD][OP_DIV]            = precedence_less;
    precedence_table[OP_ADD][IDENTIFIER]        = precedence_less;
    precedence_table[OP_ADD][LOGICAL_NOT]       = precedence_less;
    precedence_table[OP_ADD][CONDITIONAL_AND]   = precedence_less;

    precedence_table[OP_SUB][OP_LBRACE]         = precedence_less;
    precedence_table[OP_SUB][OP_MUL]            = precedence_less;
    precedence_table[OP_SUB][OP_DIV]            = precedence_less;
    precedence_table[OP_SUB][IDENTIFIER]        = precedence_less;
    precedence_table[OP_SUB][LOGICAL_NOT]       = precedence_less;
    precedence_table[OP_SUB][CONDITIONAL_AND]   = precedence_less;

    precedence_table[OP_MUL][OP_LBRACE]         = precedence_less;
    precedence_table[OP_MUL][IDENTIFIER]        = precedence_less;
    precedence_table[OP_MUL][LOGICAL_NOT]       = precedence_less;

    precedence_table[OP_DIV][OP_LBRACE]         = precedence_less;
    precedence_table[OP_DIV][IDENTIFIER]        = precedence_less;
    precedence_table[OP_DIV][LOGICAL_NOT]       = precedence_less;

    precedence_table[OP_LESS][OP_LBRACE]        = precedence_less;
    precedence_table[OP_LESS][OP_ADD]           = precedence_less;
    precedence_table[OP_LESS][OP_SUB]           = precedence_less;
    precedence_table[OP_LESS][OP_MUL]           = precedence_less;
    precedence_table[OP_LESS][OP_DIV]           = precedence_less;
    precedence_table[OP_LESS][OP_LESS]          = precedence_not_defined;
    precedence_table[OP_LESS][OP_MORE]          = precedence_not_defined;
    precedence_table[OP_LESS][OP_LESSEQUAL]     = precedence_not_defined;
    precedence_table[OP_LESS][OP_MOREEQUAL]     = precedence_not_defined;
    precedence_table[OP_LESS][IDENTIFIER]       = precedence_less;
    precedence_table[OP_LESS][LOGICAL_NOT]      = precedence_less;

    precedence_table[OP_MORE][OP_LBRACE]        = precedence_less;
    precedence_table[OP_MORE][OP_ADD]           = precedence_less;
    precedence_table[OP_MORE][OP_SUB]           = precedence_less;
    precedence_table[OP_MORE][OP_MUL]           = precedence_less;
    precedence_table[OP_MORE][OP_DIV]           = precedence_less;
    precedence_table[OP_MORE][OP_LESS]          = precedence_not_defined;
    precedence_table[OP_MORE][OP_MORE]          = precedence_not_defined;
    precedence_table[OP_MORE][OP_LESSEQUAL]     = precedence_not_defined;
    precedence_table[OP_MORE][OP_MOREEQUAL]     = precedence_not_defined;
    precedence_table[OP_MORE][IDENTIFIER]       = precedence_less;
    precedence_table[OP_MORE][LOGICAL_NOT]      = precedence_less;

    precedence_table[OP_LESSEQUAL][OP_LBRACE]       = precedence_less;
    precedence_table[OP_LESSEQUAL][OP_ADD]          = precedence_less;
    precedence_table[OP_LESSEQUAL][OP_SUB]          = precedence_less;
    precedence_table[OP_LESSEQUAL][OP_MUL]          = precedence_less;
    precedence_table[OP_LESSEQUAL][OP_DIV]          = precedence_less;
    precedence_table[OP_LESSEQUAL][OP_LESS]         = precedence_not_defined;
    precedence_table[OP_LESSEQUAL][OP_MORE]         = precedence_not_defined;
    precedence_table[OP_LESSEQUAL][OP_LESSEQUAL]    = precedence_not_defined;
    precedence_table[OP_LESSEQUAL][OP_MOREEQUAL]    = precedence_not_defined;
    precedence_table[OP_LESSEQUAL][IDENTIFIER]      = precedence_less;
    precedence_table[OP_LESSEQUAL][LOGICAL_NOT]     = precedence_less;

    precedence_table[OP_MOREEQUAL][OP_LBRACE]       = precedence_less;
    precedence_table[OP_MOREEQUAL][OP_ADD]          = precedence_less;
    precedence_table[OP_MOREEQUAL][OP_SUB]          = precedence_less;
    precedence_table[OP_MOREEQUAL][OP_MUL]          = precedence_less;
    precedence_table[OP_MOREEQUAL][OP_DIV]          = precedence_less;
    precedence_table[OP_MOREEQUAL][OP_LESS]         = precedence_not_defined;
    precedence_table[OP_MOREEQUAL][OP_MORE]         = precedence_not_defined;
    precedence_table[OP_MOREEQUAL][OP_LESSEQUAL]    = precedence_not_defined;
    precedence_table[OP_MOREEQUAL][OP_MOREEQUAL]    = precedence_not_defined;
    precedence_table[OP_MOREEQUAL][IDENTIFIER]      = precedence_less;
    precedence_table[OP_MOREEQUAL][LOGICAL_NOT]     = precedence_less;

    precedence_table[OP_EQUAL][OP_LBRACE]       = precedence_less;
    precedence_table[OP_EQUAL][OP_ADD]          = precedence_less;
    precedence_table[OP_EQUAL][OP_SUB]          = precedence_less;
    precedence_table[OP_EQUAL][OP_MUL]          = precedence_less;
    precedence_table[OP_EQUAL][OP_DIV]          = precedence_less;
    precedence_table[OP_EQUAL][OP_LESS]         = precedence_less;
    precedence_table[OP_EQUAL][OP_MORE]         = precedence_less;
    precedence_table[OP_EQUAL][OP_LESSEQUAL]    = precedence_less;
    precedence_table[OP_EQUAL][OP_MOREEQUAL]    = precedence_less;
    precedence_table[OP_EQUAL][OP_EQUAL]        = precedence_not_defined;
    precedence_table[OP_EQUAL][OP_NOTEQUAL]     = precedence_not_defined;
    precedence_table[OP_EQUAL][IDENTIFIER]      = precedence_less;
    precedence_table[OP_EQUAL][LOGICAL_NOT]     = precedence_less;
    precedence_table[OP_EQUAL][CONDITIONAL_OR]  = precedence_less;
    precedence_table[OP_EQUAL][CONDITIONAL_AND] = precedence_less;


    precedence_table[OP_NOTEQUAL][OP_LBRACE]        = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_ADD]           = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_SUB]           = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_MUL]           = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_DIV]           = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_LESS]          = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_MORE]          = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_LESSEQUAL]     = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_MOREEQUAL]     = precedence_less;
    precedence_table[OP_NOTEQUAL][OP_EQUAL]         = precedence_not_defined;
    precedence_table[OP_NOTEQUAL][OP_NOTEQUAL]      = precedence_not_defined;
    precedence_table[OP_NOTEQUAL][IDENTIFIER]       = precedence_less;
    precedence_table[OP_NOTEQUAL][LOGICAL_NOT]      = precedence_less;
    precedence_table[OP_NOTEQUAL][CONDITIONAL_OR]   = precedence_less;
    precedence_table[OP_NOTEQUAL][CONDITIONAL_AND]  = precedence_less;

    precedence_table[IDENTIFIER][IDENTIFIER]    = precedence_not_defined;
    precedence_table[IDENTIFIER][OP_LBRACE]     = precedence_not_defined;

    precedence_table[DOLLAR][OP_LBRACE]         = precedence_less;
//    precedence_table[DOLLAR][OP_RBRACE]         = precedence_not_defined;
    precedence_table[DOLLAR][OP_ADD]            = precedence_less;
    precedence_table[DOLLAR][OP_SUB]            = precedence_less;
    precedence_table[DOLLAR][OP_MUL]            = precedence_less;
    precedence_table[DOLLAR][OP_DIV]            = precedence_less;
    precedence_table[DOLLAR][OP_LESS]           = precedence_less;
    precedence_table[DOLLAR][OP_MORE]           = precedence_less;
    precedence_table[DOLLAR][OP_LESSEQUAL]      = precedence_less;
    precedence_table[DOLLAR][OP_MOREEQUAL]      = precedence_less;
    precedence_table[DOLLAR][OP_EQUAL]          = precedence_less;
    precedence_table[DOLLAR][OP_NOTEQUAL]       = precedence_less;
    precedence_table[DOLLAR][IDENTIFIER]        = precedence_less;
    precedence_table[DOLLAR][DOLLAR]            = precedence_not_defined;
    precedence_table[DOLLAR][LOGICAL_NOT]       = precedence_less;
    precedence_table[DOLLAR][CONDITIONAL_OR]    = precedence_less;
    precedence_table[DOLLAR][CONDITIONAL_AND]   = precedence_less;

    precedence_table[LOGICAL_NOT][OP_LBRACE]    = precedence_less;
    precedence_table[LOGICAL_NOT][LOGICAL_NOT]  = precedence_less;
    precedence_table[LOGICAL_NOT][IDENTIFIER]   = precedence_less;

    precedence_table[CONDITIONAL_OR][OP_LBRACE]         = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_ADD]            = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_SUB]            = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_MUL]            = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_DIV]            = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_LESS]           = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_MORE]           = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_LESSEQUAL]      = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_MOREEQUAL]      = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_EQUAL]          = precedence_less;
    precedence_table[CONDITIONAL_OR][OP_NOTEQUAL]       = precedence_less;
    precedence_table[CONDITIONAL_OR][IDENTIFIER]        = precedence_less;
    precedence_table[CONDITIONAL_OR][CONDITIONAL_AND]   = precedence_less;
    precedence_table[CONDITIONAL_OR][LOGICAL_NOT]       = precedence_less;

    precedence_table[CONDITIONAL_AND][OP_LBRACE]         = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_ADD]            = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_SUB]            = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_MUL]            = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_DIV]            = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_LESS]           = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_MORE]           = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_LESSEQUAL]      = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_MOREEQUAL]      = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_EQUAL]          = precedence_less;
    precedence_table[CONDITIONAL_AND][OP_NOTEQUAL]       = precedence_less;
    precedence_table[CONDITIONAL_AND][IDENTIFIER]        = precedence_less;
    precedence_table[CONDITIONAL_AND][LOGICAL_NOT]       = precedence_less;
}

int compareRules(int defined_rules[NUMBER_OF_RULES][3], int rule_to_compare[3]) {
    int elements;
    for (int rule_number = 0; rule_number < NUMBER_OF_RULES; ++rule_number) {
        elements = 0;
        for (int i = 0; i < 3; ++i) {
            if (defined_rules[rule_number][i] == rule_to_compare[i]){
                elements++;
                if (elements == 3)
                    return rule_number;
            }
        }
    }
    return -1;
}

entry_s *generateEmptyConstant(data_value_type type) {
    table_data_value constant;
    constant.constant.type = type;

    tab_t_data *tab_data = myMalloc(sizeof(tab_t_data));
    tab_data->type = tTypeConstant;
    tab_data->value = constant;

    static int i = 0; char tmp[100];
    int length = 0;

    if (i < 10)
        length = 1;
    else if (i < 100)
        length = 2;
    else if (i < 1000)
        length = 3;
    else if (i < 10000)
        length = 4;

    sprintf(tmp, "%d", i);
    tmp[length] = '\0';

    char *key = myMalloc(strlen("int_const") + strlen(tmp) +1 );
    strcat(key, "int_const");
    strcat(key, tmp);
    i++;

    entry_s *item;
    if (phase == 2){
        htInsert(ListOfSymbolsTables->destinationSymbolTable, key, tab_data);
        item = htSearch(ListOfSymbolsTables->destinationSymbolTable, key);
    } else {
        item = myMalloc(sizeof(entry_s));
        item->key = key;
        item->data = tab_data;
        item->next = NULL;
    }
    return item;
}

void generateConstant(tPrecStack *ptrstack, tPrecStack *helpstack, t_hashtable *hashtable){

    /*
     * create new constant (symbol)
     */
    table_data_value constant;
    constant.constant.type = stateToValueType( (helpstack->arr[helpstack->top])->token.state );

    /*
     * Insert value to symbol
     */
    if ((helpstack->arr[helpstack->top])->token.state == s_int){
        // char* -> int
        constant.constant.value.t_int = atoi((helpstack->arr[helpstack->top])->token.data);
    } else if ((helpstack->arr[helpstack->top])->token.state == s_double){
        // char* -> double
        constant.constant.value.t_double = atof((helpstack->arr[helpstack->top])->token.data);
    } else if ( (helpstack->arr[helpstack->top])->token.state == s_string ){
        // char* -> string
        String str;
        init_value_str(&str, (helpstack->arr[helpstack->top])->token.data); //from str.c
        constant.constant.value.str = str;
    } else if ( (helpstack->arr[helpstack->top])->token.state == s_boolean ){

        if (strcmp((helpstack->arr[helpstack->top])->token.data, "true") == 0)
            constant.constant.value.t_int = 1;
        else if (strcmp((helpstack->arr[helpstack->top])->token.data, "false") == 0)
            constant.constant.value.t_int = 0;
    }

    /*
     * create new symbol
     */
    tab_t_data *tab_data = myMalloc(sizeof(tab_t_data));
    tab_data->type = tTypeConstant;
    tab_data->value = constant;

    entry_s *item;
    if (phase == 2){
        /*
         * only in phase 2 generate symbols and instructions
         */
        // insert new symbol to symbol table //todo check if hashtable is correct destination
        htInsert(hashtable, (helpstack->arr[helpstack->top])->token.data, tab_data);

        // keep pointer to new symbol
        item = htSearch(hashtable, (helpstack->arr[helpstack->top])->token.data);
#ifdef _DEBUG_PRECEDENCE
//        table_print(hashtable);
#endif
    } else {
        /*
         * generate local symbol, NOT inserted into hashtable in phase 1
         */
        item = myMalloc(sizeof(entry_s));
        item->key = (helpstack->arr[helpstack->top])->token.data;
        item->data = tab_data;
        item->next = NULL;
    }

    // insert expression with pointer to item (symbol - constant) to stack
    stackPush( ptrstack, s_expression, (ptrstack->arr[ptrstack->top])->token, item);

}

void generateFullInstruction(tPrecStack *helpstack, instructions instType, tToken token, tPrecStack *ptrstack) {

    //On the top of helpstack is '<'
    stackPop( helpstack ); // pop '<' out
    /*
     * Operand 1
     */
    entry_s *operand1 = (helpstack->arr[helpstack->top])->item;
    data_value_type op1_type = (helpstack->arr[helpstack->top])->item->data->value.constant.type;
    stackPop( helpstack ); // pop first symbol out

    stackPop( helpstack ); //pop instruction type

    /*
     * Operand 2
     */
    entry_s *operand2 = (helpstack->arr[helpstack->top])->item;
    data_value_type op2_type = (helpstack->arr[helpstack->top])->item->data->value.constant.type;
    stackPop( helpstack ); // pop second symbol out

    /*
     * Type inspection
     */
    int type = instAddTypes(op1_type, op2_type, instType);

    if (type == -1) {
#ifdef _DEBUG_PRECEDENCE
        printf("ERROR - type mismatch");
#endif
        freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
    }

    /*
     * TODO quick bugfix
     */
    data_value_type const_type;
    switch (type){
        case 0:
            const_type = valueType_double;
            break;
        case 1:
            const_type = valueType_string;
            break;
        case 2:
            const_type = valueType_int;
            break;
        case 3:
            const_type = valueType_boolean;
            break;
        default:
            const_type = valueType_int; //todo quick bugfix
            break;
    }

    //create a destination symbol with correct type

    entry_s* destination;
    if (phase == 2){
        /*
         * only in phase 2 generate symbols and instructions
         */
        destination = generateEmptyConstant(const_type);

//        if (op1_type != op2_type && const_type == valueType_double){ //const_type of destination
//            if (op1_type == valueType_int)
//                operand1 = intToDouble(operand1); // operand1 to double
//            else
//                operand2 = intToDouble(operand2); // operand2 to double
//        }
    } else {
        /*
         * generate local symbol, NOT inserted into hashtable in phase 1
         */
        table_data_value constant;
        constant.constant.type = const_type;

        tab_t_data *tab_data = myMalloc(sizeof(tab_t_data));
        tab_data->type = tTypeConstant;
        tab_data->value = constant;

        destination = myMalloc(sizeof(entry_s));
        char * key = "intern_constant";
        destination->key = key;
        destination->data = tab_data;
        destination->next = NULL;
    }

    /*
     * keep pointer to instruction.destination in new expression
     */
    stackPush(ptrstack, s_expression, token, destination);


    if (phase == 2){
        /*
         * Create new instruction
         */

        //I_ADD vs I_CONCATENATE
        if (instType == I_ADD && const_type == valueType_string)
            instType = I_CONCATENATE;

        tInstr instruction; //I_LESS / dst / op1 / op2
        instruction.instType = instType;
        instruction.addr1 = destination; //new expression
        instruction.addr2 = operand1;
        instruction.addr3 = operand2;

        /*
         * Insert instruction to instruction list
         */
//        printf("cicina je spatky \n");
//        print_elements_of_list(*ListOfSymbolsTables->instList);
        listInsertLast(ListOfSymbolsTables->instList, instruction);

//        printf("after insert instruction");
    }

//    printf("operand1 key: %s \n", operand1->key);
//    printf("operand1: %d \n", operand1->data->type);
//    printf("operand1: %d \n", operand1->data->value.constant.type);

//    entry_s *frst = instList->first->Instruction.addr2;
//    entry_s *scnd = instList->first->Instruction.addr3;

//    printf("inst type: %d \n", instList->first->Instruction.instType);
//    printf("frst value: %s \n", frst->key);
//    printf("scnd value: %s \n", scnd->key);
}

instructions ruleToInstruction(int rule_number){

    switch (rule_number){
        case 0: /* E -> E + E   No.0 */
            return I_ADD;
        case 1: /* E -> E - E   No.1 */
            return I_SUB;
        case 2: /* E -> E * E   No.2 */
            return I_MUL;
        case 3: /* E -> E / E   No.3 */
            return I_DIV;
        case 4: /* E -> E < E   No.4 */
            return I_LESS;
        case 5: /* E -> E > E   No.5 */
            return I_MORE;
        case 6: /* E -> E <= E  No.6 */
            return I_LESS_EQUAL;
        case 7: /* E -> E >= E  No.7 */
            return I_MORE_EQUAL;
        case 8: /* E -> E == E  No.8 */
            return I_EQUAL;
        case 9: /* E -> E != E  No.9 */
            return I_NOT_EQUAL;
        case 12:/* E -> !E      No.12*/
            return I_NOT;
        case 13:/* E -> E || E  No.13*/
            return I_OR;
        case 14:/* E -> E && E  No.14*/
            return I_AND;
        default:
            break; // can't reach this state
    }
}

int generateRuleFromStack(int **defined_rules, tPrecStack *ptrstack, tToken temp_token, t_hashtable *hashtable) {

    /*
     * Declare som things
     */
    tData data; //stack data
    int new_rule[3]; // new rule
    data.entry = -1; //reset new rule

    tPrecStack* helpstack;
    //todo bug s alokaciou
//    helpstack = (tPrecStack*) myMalloc(sizeof(tPrecStack) + STACK_INIT_SIZE*sizeof(tData *));
    helpstack = (tPrecStack*) malloc(sizeof(tPrecStack) + STACK_INIT_SIZE*sizeof(tData *));
    stackInit( helpstack );

    /*
     * Create new rule for compare if truly exist
     */
    int index = 0;
    while ( (data.entry != precedence_less) ) {

        if (data.entry == s_dollar) //prevent from infinite loop in special scenarios
            break;

        stackTop( ptrstack, &data );
        stackPush( helpstack, data.entry, data.token, data.item); //temporary store in help stack
        stackPop( ptrstack );
        new_rule[index] = data.entry;
        index++;
    }

    if (index == 1)
        new_rule[1] = -1;
    else if (index == 2){
        new_rule[2] = -1;
        new_rule[1] = -1;
    } else if (index == 3){
        new_rule[2] = -1;
    }

//    printf("print rule prototyp: %d %d %d \n", new_rule[0], new_rule[1], new_rule[2]);

    /*
     * Check if rule exist
     */
    int rule_number = compareRules(defined_rules, new_rule);

    instructions instruction_number = ruleToInstruction(rule_number);
#ifdef _DEBUG_PRECEDENCE
//        printf("instruction number: %d \n", instruction_number);
#endif
    switch (rule_number){
        case -1: // rule does not exist - return data from helpstack to ptrstack
            while ( !stackEmpty(helpstack) ){
                stackTop( helpstack, &data );
                stackPush( ptrstack, data.entry, data.token, data.item);
                stackPop( helpstack );
            }
            break;
        /*
         * Addition and Concatenation
         */
        case 0: //E -> E + E

        /*
         * Mathematics operation - type inspection
         */
        case 1: //E -> E - E
        case 2: //E -> E * E
        case 3: //E -> E / E

        /*
         * Comparisons operation
         * destination type: valueType_boolean
         */
        case 4: // E -> E < E
        case 5: // E -> E > E
        case 6: // E -> E <= E
        case 7: // E -> E >= E
        case 8: // E -> E == E
        case 9: // E -> E != E
            generateFullInstruction(helpstack, instruction_number, temp_token, ptrstack);
            break;
        case 10: //E -> (E)
            /*
             *  no type inspection, expression inherit type
             *  helpstack is holding pointer to symbol
             *  not generating instruction here
             */
            stackPop( helpstack ); // pop '<' out
            stackPop( helpstack ); // pop ( out
            stackPush(ptrstack, s_expression, temp_token, (helpstack->arr[helpstack->top])->item);
            break;

        case 11: //E -> i
            /*
             * no type inspection, expression inherit type
             * fixme pri premennych musim zistovat type z tabulky symbolov, nie z tokenu !!!!
             */
            //On the top of helpstack is '<'
            stackPop( helpstack ); // pop '<' out

            // determine if we got constant or variable or classified identified
            if (phase == 2 && (helpstack->arr[helpstack->top])->token.state == s_identifier){

                entry_s *item = NULL;
                item = htSearch(ListOfSymbolsTables->functionSymbolTable, (helpstack->arr[helpstack->top])->token.data);
                if (item == NULL){
                    item = htSearch(ListOfSymbolsTables->classSymbolTable, (helpstack->arr[helpstack->top])->token.data);
                    if (item == NULL){
                        printf("ERROR - houston, mame problem\n");
                        freeAndExit(INTERNAL_ERROR);
                    }
                }
                stackPush( ptrstack, s_expression, (ptrstack->arr[ptrstack->top])->token, item);

            } else if (phase == 2 && (helpstack->arr[helpstack->top])->token.state == s_classified_identifier){
                // classified identifier, use function from parser to find item

                parseClassifiedIdentifier( &(helpstack->arr[helpstack->top])->token , classifiedIdentifier);
                t_hashtable *tempTable = classifiedIdentifier->classSymbolTable;
                entry_s *item = htSearch(tempTable, classifiedIdentifier->var_identifier);
                stackPush( ptrstack, s_expression, (ptrstack->arr[ptrstack->top])->token, item);

            } else {
                generateConstant(ptrstack, helpstack, hashtable);
            }

            break;

        /*
         * pretoze to nie je klasicka instukcia s 2 operandmi, generovanie instrukcie je custom
         */
        case 12: //E -> !E   BOOLEAN extension
            stackPop( helpstack ); // pop '<' out
            stackPop( helpstack ); // pop '!' out

            /*
             * Operand
             */
            entry_s *operand = (helpstack->arr[helpstack->top])->item;
            // type inspection
            if ((helpstack->arr[helpstack->top])->item->data->value.constant.type != valueType_boolean)
                freeAndExit(SEMANTIC_ERROR_TYPE_COMPATIBILITY);
            stackPop( helpstack ); // pop symbol out

            //create a destination symbol with correct type - boolean
            entry_s* destination;
            destination = generateEmptyConstant(valueType_boolean);

            /*
             * keep pointer to instruction.destination in new expression
             */
            stackPush(ptrstack, s_expression, temp_token, destination);

            if (phase == 2) {
                /*
                 * Create new instruction
                 */
                tInstr instruction; //I_NOT / dst / op1 / -
                instruction.instType = I_NOT;
                instruction.addr1 = destination; //new expression
                instruction.addr2 = operand;
                instruction.addr3 = NULL;

                /*
                 * Insert instruction to instruction list
                 */
                listInsertLast(ListOfSymbolsTables->instList, instruction);
            }

            break;
        case 13: //E -> E || E BOOLEAN extension
        case 14: //E -> E && E BOOLEAN extension
            generateFullInstruction(helpstack, instruction_number, temp_token, ptrstack);
            break;
        /*
         * END of boolean operations
         */

        default: //do nothing
            break;
    }

    free( helpstack);

    return rule_number;
}

void customPrintState(int cislo){
    switch(cislo){
        case 0:
            printf(" '>' ");
            break;

        case 1:
            printf(" (   ");
            break;

        case 2:
            printf(" )   ");
            break;

        case 3:
            printf(" +   ");
            break;

        case 4:
            printf(" -   ");
            break;

        case 5:
            printf(" *   ");
            break;

        case 6:
            printf(" /   ");
            break;

        case 7:
            printf(" <   ");
            break;

        case 8:
            printf(" >   ");
            break;

        case 9:
            printf(" <=  ");
            break;

        case 10:
            printf(" >=  ");
            break;

        case 11:
            printf(" ==  ");
            break;

        case 12:
            printf(" !=  ");
            break;

        case 13:
            printf(" i   ");
            break;

        case 14:
            printf(" $   ");
            break;

        case 15:
            printf(" E   ");
            break;

        case 16:
            printf(" &&  ");
            break;

        case 17:
            printf(" ||  ");
            break;

        case 18:
            printf(" !   ");
            break;

        case 19:
            printf(" '<' ");
            break;

        case 20:
            printf(" '=' ");
            break;

        case 21:
            printf(" 'e' "); //precedence_not_defined
            break;

        case 34:
            printf(" 's_int' ");
            break;

        case 35:
            printf(" 's_double' ");
            break;

        case 36:
            printf(" 's_string' ");
            break;

        case 37:
            printf(" 's_boolean' ");
            break;

        default:
            printf(" unknown ");
            break;
    }
}

void customPrintRule(int cislo){
    switch(cislo) {
        case 0:
            printf(" E <- E + E ");
            break;
        case 1:
            printf(" E -> E - E ");
            break;
        case 2:
            printf(" E -> E * E ");
            break;
        case 3:
            printf(" E -> E / E ");
            break;
        case 4:
            printf(" E -> E < E ");
            break;
        case 5:
            printf(" E -> E > E ");
            break;
        case 6:
            printf(" E -> E <= E ");
            break;
        case 7:
            printf(" E -> E >= E ");
            break;
        case 8:
            printf(" E -> E == E ");
            break;
        case 9:
            printf(" E -> E != E ");
            break;
        case 10:
            printf(" E -> (E) ");
            break;
        case 11:
            printf(" E -> i ");
            break;
        case 12:
            printf(" E -> !E ");
            break;
        case 13:
            printf(" E -> E || E ");
            break;
        case 14:
            printf(" E -> E && E ");
            break;
        default:
            printf(" unknown rule \n");
            break;
    }
}

int endOfExpression(int state){

    /*
     * Determine end of expression
     */
    switch (state){
//        case s_semicolon:
//        case s_eof:
//        case s_comma:
//        case s_left_vinculum:
//        case s_right_vinculum:
//            return s_dollar;

        case s_int:
        case s_double:
        case s_string:
        case s_boolean:
        case s_classified_identifier:
            return s_identifier;

        case s_exclamation:
            return s_binary_not;

        case precedence_more:
        case s_left_bracket:
        case s_right_bracket:
        case s_plus:
        case s_minus:
        case s_multiply:
        case s_divide:
        case s_less:
        case s_more:
        case s_less_equal:
        case s_more_equal:
        case s_equal_expression:
        case s_diff_expression:
        case s_identifier:
        case s_dollar:
        case s_expression:
        case s_binary_and:
        case s_binary_or:
        case s_binary_not:
            return state;

        default:
            return s_dollar;
    }

//    if (state == s_semicolon)
//        return s_dollar;
//    else if (state == s_eof)
//        return s_dollar;
//    else if (state == s_comma)
//        return s_dollar;
//    else if (state == s_left_vinculum)
//        return s_dollar;
//    else if (state == s_right_vinculum)
//        return s_dollar;

    /*
     * Data types -> identifiers
     */
//    else if (state == s_int)
//        return s_identifier;
//    else if (state == s_double)
//        return s_identifier;
//    else if (state == s_string)
//        return s_identifier;
//    else if (state == s_boolean)
//        return s_identifier;
//
//    else if (state == s_classified_identifier)
//        return s_identifier;

//    else if (state == s_exclamation)
//        return s_binary_not;
//
//    else
//        return state;
}

entry_s *precedenceCore(int parser_phase) {


phase = parser_phase;

#ifdef _DEBUG_PRECEDENCE
    printf("\n\n^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
    printf("precedence Core \n");

    printf("state: %d \n", phase);
#endif

    /*
     * Precedence table init
     */
    precedence_init(precedence_table);
#ifdef _DEBUG_PRECEDENCE
//    printPrecedenceTable(precedence_table);
#endif

    /*
     * Stack init
     */
    tPrecStack* ptrstack;
    tPrecStack* helpstack;
//    int STACK_SIZE = 8;
    ptrstack = (tPrecStack*) myMalloc(sizeof(tPrecStack) + STACK_INIT_SIZE*sizeof(tData *));
    helpstack = (tPrecStack*) myMalloc(sizeof(tPrecStack) + STACK_INIT_SIZE*sizeof(tData *));
    stackInit( ptrstack );
    stackInit( helpstack );

    /*
     * Declare some staff
     */
    tToken *temp_token = myMalloc(sizeof(tToken));
    tData data;
    tData helpdata;
    int rule_number;
    int a;
    int braces = 0;

    t_hashtable *functionSymbolTable = ListOfSymbolsTables->functionSymbolTable;
    t_hashtable *classSymbolTable = ListOfSymbolsTables->classSymbolTable;
    t_hashtable *destinationSymbolTable = ListOfSymbolsTables->destinationSymbolTable;

//    tListOfInstr *mylist = ListOfSymbolsTables->instList;
//    printf("%p \n", ListOfSymbolsTables->instList);
//    print_elements_of_list(*mylist);
//    exit(0);


    /*
     * Vlož $ na zásobník
     */
    token = getNextTokenFromScanner();
    stackPush( ptrstack, s_dollar, *token, NULL);

    /*
     * b = aktuální znak na vstupu
     */
    int b = token->state;
    b = endOfExpression(b); // zamen ; za $

    /*
     * Handling " ) " at the end of expression (functions, if, while)
     */
    if (b == s_left_bracket)
        braces++;

    if (b == s_right_bracket){
        braces--;
        if (braces < 0)
            b = s_dollar;
    }

    int end_of_loop = 1;
    do{
        /*
         * a = stack top
         */
        a = stackTopTerminal( ptrstack, &data );

#ifdef _DEBUG_PRECEDENCE
        printf("token: \" %s \" \n", token->data);
//        printf("token state: \" %d \" \n", token->state);
#endif

        int action = precedence_table[a][b]; //Tabulka[a, b]

#ifdef _DEBUG_PRECEDENCE
//        printf("a: ");
//        printf("%d \n", a);
//        customPrintState(a);
//        printf("\n");
//
//        printf("b: ");
//        printf("%d", b);
//        customPrintState(b);
//        printf("\n");
//
//        printf("action: %d \n", action);
#endif
        switch(action) {
            case precedence_equal: // =

                /*
                 * push(b)
                 */
                stackPush( ptrstack, b, *token, NULL);

#ifdef _DEBUG_PRECEDENCE
                stackPrint(ptrstack);
#endif

                /*
                 * read another symbol 'b' from input
                 */
                token = getNextTokenFromScanner();
                if (token == NULL)
                    break;
                b = token->state;
                b = endOfExpression(b); // zamen ; za $ a podobne


                /*
                 * Handling " ) " at the end of expression (functions, if, while)
                 */
                if (b == s_left_bracket)
                    braces++;

                if (b == s_right_bracket){
                    braces--;
                    if (braces < 0)
                        b = s_dollar;
                }

#ifdef _DEBUG_PRECEDENCE
                printf("precedence_equal \n\n");
#endif
                break;

            case precedence_less: // <
                /*
                 * zaměň a za a< na zásobníku
                 */
                //keep token value for expression
                temp_token->data = myMalloc(strlen(token->data)+1 );
                memcpy(temp_token->data, token->data, strlen(token->data)+1 );
                temp_token->state = token->state;

                // get elements from stack until we reach terminal
                while ( (ptrstack->arr[ptrstack->top])->entry !=  stackTopTerminal( ptrstack, &data ) ){
                    stackTop( ptrstack, &helpdata );
                    stackPush( helpstack, helpdata.entry, helpdata.token, helpdata.item); //temporary store in help stack
                    stackPop( ptrstack );
                }

                // insert < after terminal
                stackPush( ptrstack, precedence_less, *token, NULL);

                //return data to main stack
                while ( !stackEmpty( helpstack ) ){
                    stackTop( helpstack, &helpdata );
                    stackPush( ptrstack, helpdata.entry, helpdata.token, helpdata.item);
                    stackPop( helpstack );
                }

                /*
                 * push(b)
                 */
                if (token->state == s_identifier && phase == 2){
                    /*
                     * Look into symbol table only in phase 2
                     */

                    entry_s *item = NULL; //variable from symbol table (declaration)

                    if (ListOfSymbolsTables->data_type == tTypeFunction){
                        functionSymbolTable = ListOfSymbolsTables->functionSymbolTable;
#ifdef _DEBUG_PRECEDENCE
                        table_print(functionSymbolTable);
#endif
                        //Search in functionSymbolTable
                        item = htSearch(functionSymbolTable, token->data);
                        if (item == NULL){ // variable not found in functionSymbolTable
                            //look into classSymbolTable
                            classSymbolTable = ListOfSymbolsTables->classSymbolTable;
                            item = htSearch(classSymbolTable, token->data);
                            if (item == NULL){ // variable not found
                                freeAndExit(SEMANTIC_ERROR);
                            }
                        }
                    } else if (ListOfSymbolsTables->data_type == tTypeClass){
                        //look into classSymbolTable
                        classSymbolTable = ListOfSymbolsTables->classSymbolTable;
#ifdef _DEBUG_PRECEDENCE
                        table_print(classSymbolTable);
#endif
                        item = htSearch(classSymbolTable, token->data);

                        if (item == NULL){ // variable not found
                            freeAndExit(SEMANTIC_ERROR);
                        }
                    } else {
                        printf("\n\n Mareckuuuuu, co mi to tu posielas ???!?!?!??!!? \n\n");
                        freeAndExit(INTERNAL_ERROR);
                    }

                    //insert item into stack
                    stackPush( ptrstack, token->state, *temp_token, item);

                } else if (token->state == s_classified_identifier && phase == 2){
//                    printf("narazili sme na klasifikovany pristup\n");
                    parseClassifiedIdentifier(token, classifiedIdentifier);
                    t_hashtable *tempTable = classifiedIdentifier->classSymbolTable;
                    entry_s *item = htSearch(tempTable, classifiedIdentifier->var_identifier);

#ifdef _DEBUG_PRECEDENCE
//                    printf("class identifier: %s \n", classifiedIdentifier->class_identifier);
//                    printf("var identifier: %s \n", classifiedIdentifier->var_identifier);
//                    printf("tempTable \n");
//                    table_print(tempTable);
//                    printf("item key: %s \n", item->key);
#endif

                    //insert item into stack
                    //token->state is s_classified_identifier, push s_identifier instead
                    stackPush( ptrstack, s_identifier, *temp_token, item);

                } else {
                    /*
                     * Otherwise don't push symbol to stack
                     */
                    stackPush( ptrstack, b, *temp_token, NULL);
                }

#ifdef _DEBUG_PRECEDENCE
                stackPrint(ptrstack);
#endif

                /*
                 * read another symbol 'b' from input
                 */
//                printf("token before getNextTokenFromScanner: \" %s \" \n", token->data);
                token = getNextTokenFromScanner();
//                printf("token after getNextTokenFromScanner: \" %s \" \n", token->data);
                b = token->state;
                b = endOfExpression(b); // zamen ; za $

                /*
                 * Handling " ) " at the end of expression (functions, if, while)
                 */
                if (b == s_left_bracket)
                    braces++;


                if (b == s_right_bracket){
                    braces--;
                    if (braces < 0)
                        b = s_dollar;
                }


#ifdef _DEBUG_PRECEDENCE
//                printf("precedence less: ");
//                printf("%d", b);
//                customPrintState(b);
//                printf("\n");

                printf("precedence_less\n\n");
#endif

                break;
            case precedence_more: // >
                /*
                 * if '<y' in on the top of the stack and 'r: A → y ∈ P'
                 * then exchange '<y' for 'A' & vypiš r na výstup
                 * else error
                 */
                rule_number = generateRuleFromStack(pole, ptrstack, *temp_token, symbol_table);

#ifdef _DEBUG_PRECEDENCE
                printf("rule No. %d || ", rule_number);
                customPrintRule(rule_number);
                printf("\n");
#endif

                if (rule_number == -1)
                    freeAndExit(SYNTAX_ERROR); // 2


#ifdef _DEBUG_PRECEDENCE
                stackPrint(ptrstack);
                printf("precedence_more \n\n");
#endif
                break;

            case precedence_not_defined:
                /*
                 * prázdné políčko : chyba
                 * todo poriadne otestovat zdrojak.forward.declaration.java
                 */
#ifdef _DEBUG_PRECEDENCE
                printf("precedence_not_defined \n\n");
#endif
                freeAndExit(SYNTAX_ERROR); // 2

            default:
                break; //do nothing
        }//end switch

    if ( stackTopTerminal(ptrstack,&data)==s_dollar && b==s_dollar )
        end_of_loop = 0;

    }
    while( end_of_loop );

#ifdef _DEBUG_PRECEDENCE
    printf("last token: \" %s \" \n", token->data);
//
//    printf("\n global symbol_table: \n");
//    table_print(symbol_table);
//    printf("\n");
//
//    if (phase == 2 && classSymbolTable != NULL){
//        printf("\n classSymbolTable: \n");
//        table_print(classSymbolTable);
//        printf("\n");
//    }
////
//    if (phase == 2 && functionSymbolTable != NULL) {
//        printf("\n functionSymbolTable: \n");
//        table_print(functionSymbolTable);
//        printf("\n");
//    }
////
//    if (phase == 2 && destinationSymbolTable != NULL) {
//        printf("\n destinationSymbolTable: \n");
//        table_print(destinationSymbolTable);
//        printf("\n");
//    }
#endif

//    table_print(ListOfSymbolsTables->destinationSymbolTable);

    /*
     * Return destination to parser
     */
    return (ptrstack->arr[ptrstack->top])->item;
}

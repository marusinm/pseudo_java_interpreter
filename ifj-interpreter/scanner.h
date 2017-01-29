/* **************************** scanner.h *********************************** */
/* Soubor:              scanner.h                                             */
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

#ifndef IFJ2016_SCANNER_H
#define IFJ2016_SCANNER_H

#define BUFFER_SIZE 10 //velkost bufferu pre navratene tokeny
FILE *file;

// boolean, break, class, continue, do, double, else, false, for,
// if, int, return, String, static, true, void, while.


//extern tToken *token;
//states
typedef enum {

    /*
     * Precedence state
     */
            precedence_more, // > (0)

    /*
     * Precedence table
     */
            s_left_bracket,     // '(' (1)
    s_right_bracket,    // ')' (2)
    s_plus,             // '+' (3)
    s_minus,            // '-' (4)
    s_multiply,         // '*' (5)
    s_divide,           // '/' (6)
    s_less,             // '<' (7)
    s_more,             // '>' (8)
    s_less_equal,       // '<=' (9)
    s_more_equal,       // '>=' (10)
    s_equal_expression, // '==' (11)
    s_diff_expression,  // '!=' (12)
    s_identifier,       // 'i'  (13)
    s_dollar,           // $ (14) - only for precedence
    s_expression,       // E (15) - only for precedence || neterminal

    /*
     * Assignment extension
     */
            s_binary_and,       // '&&' (16)
    s_binary_or,        // '||' (17)
    s_binary_not,       // '!'  (18)

    /*
     * Other precedence states
     */
            precedence_less,    // < (19) || neterminal
    precedence_equal,   // = (20)
    precedence_not_defined, //?? (21)

    /*
     * Errors
     */
            s_lex_err, // (22)

    /*
     * Operators
     */
            s_exclamation, // 23
    s_equal_sign,  // 24


    /*
     * Characters
     */
            s_left_vinculum,  // '{' 25
    s_right_vinculum, // '}' 26
    s_semicolon,      // ';' 27
    s_colon,          // ':' 28
    s_comma,          // ',' 29


    /*
     * Keywords, identifier start-end-states
     */
            s_eof,            // 30
    s_start,          // 31
    s_end,            // 32
    s_keyword,        // 33


    /*
     * Data types
     */
            s_int,            // 34
    s_double,         // 35
    s_string,         // 36
    s_boolean,        // 37

    /*
     * Helper states for lexikal analization
     */

    // numbers helpers
            s_helper_first_zero,    // 29 '0'
    s_helper_real_number,   // 30 '0-9'
    s_helper_exp_char,      // 31 'e,E'
    s_helper_double,        // 32 '0-9.0-9'
    s_helper_exp_sign,      // 33 '9.9+e99 / 9.9-e99'
    s_helper_zero_to_nine,  // 34 '0-9'
    s_helper_exp_first_zero,// 35 '0'

    // string helpers
            s_helper_string_start,      // 36
    s_helper_string_one_to_nine,// 37
            s_helper_string_escape,     // 38
    s_helper_string_zeros,      // 39

    // comment helpers
            s_helper_divide_or_comment,   // 40
    s_helper_simple_comment,      // 41
    s_helper_block_comment_start, // 42
    s_helper_block_comment_escape,// 43

    s_classified_identifier,      // 44
    s_helper_identifier,          // 45


    s_comment_start,

    /*
        *
        * special characters
        *
        */
    s_special_character,
    s_helper_spec_one_two,
    s_helper_spec_third_state,
    s_helper_spec_three,
    s_helper_spec_seven,
    s_helper_spec_second_zero,
    s_helper_spec_third_zero,
    s_helper_identifier_isSpace,

} tState;

//token and informations about them
typedef struct {
    char *data;         // data
    int num_of_chars;   // number of characters in token
    int allocate_mem;   // size of allocate memory
    tState state;
} tToken;

//char *keyWords[]; // array of keywords

tToken* initToken(tToken *);

tToken* editToken(tToken *, int character);

void clearToken(tToken *);

tToken* getNextTokenFromScanner();

tToken* getNextToken();


/**
 *
 * @brief copy source file from main to our scanner
 *
 */

void setSourceFile(FILE *f);

//najvacsi rollback tokenov v rekurzivnom zostupe je 5 (pri priradeni funkcie cez klasifikovany sposob do premennej)
//pre istotu som nastavil poole na 10 prvkov - TODO: treba dohodnut este s precedencnou analyzov
tToken* tokensBuffer[BUFFER_SIZE];

/**
 * @brief funkcia sluzi na ulozenie navrateneho tokenu do bufferu typu FIFO
 * @return 0 - pokial sa nepodarilo ulozit do bufferu
 */
int returnTokenAndSaveToBuffer(tToken * returned_token);

void initBuffer();



#endif
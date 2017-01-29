/* **************************** scanner.c *********************************** */
/* Soubor:              scanner.c                                             */
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
#include <ctype.h>
#include <string.h>
#include <stdbool.h>
#include "scanner.h"
#include "common.h"

#define NUM_OF_KEYWORDS 15
#define INTERPRETER_ERROR (99)
#define LEXICAL_ERROR (1)
#define LEX_ERR 1
//#define _DEBUG

// new pointer to file in main at ex.
//FILE *file;

char *keyWords[] = { "boolean", "break", "class", "continue", "do", "double", "else", "for",
                     "if", "int", "return", "String", "static", "void", "while" };
char *boolValues[] = {"true", "false"};

int ascii[378] = {'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007', '\008', '\009', '\010',
                  '\011', '\012', '\013', '\014', '\015', '\016', '\017', '\018', '\019', '\020',
                  '\021', '\022', '\023', '\024', '\025', '\026', '\027', '\028', '\029', '\030',
                  '\031', '\032', '\033', '\034', '\035', '\036', '\037', '\038', '\039', '\040',
                  '\011', 'gggg', '\043', '\044', '\045', '\046', '\047', '\048', '\049', '\050',
                  '\051', '\052', '\053', '\054', '\055', '\056', '\057', '\058', '\059', '\060',
                  '\061', '\062', '\063', '\064', '\065', '\066', '\067', '\068', '\069', '\070',
                  '\071', '\072', '\073', '\074', '\075', '\076', '\077', '\078', '\079', '\080',
                  '\081', '\082', '\083', '\084', '\085', '\086', '\087', '\088', '\089', '\090',
                  '\091', '\092', '\093', '\094', '\095', '\096', '\097', '\098', '\099', '\100',
                  '\101', '\102', '\103', '\104', '\105', '\106', '\107', '\108', '\109', '\110',
                  '\111', '\112', '\113', '\114', '\115', '\116', '\117', '\118', '\119', '\120',
                  '\121', '\122', '\123', '\124', '\125', '\126', '\127', '\128', '\129', '\130',
                  '\131', '\132', '\133', '\134', '\135', '\136', '\137', '\138', '\139', '\140',
                  '\111', '\142', '\143', '\144', '\145', '\146', '\147', '\148', '\149', '\150',
                  '\151', '\152', '\153', '\154', '\155', '\156', '\157', '\158', '\159', '\160',
                  '\161', '\162', '\163', '\164', '\165', '\166', '\167', '\168', '\169', '\170',
                  '\171', '\172', '\173', '\174', '\175', '\176', '\177', '\178', '\179', '\180',
                  '\181', '\182', '\183', '\184', '\185', '\186', '\187', '\188', '\189', '\190',
                  '\191', '\192', '\193', '\194', '\195', '\196', '\197', '\198', '\199', '\200',
                  '\201', '\202', '\203', '\204', '\205', '\206', '\207', '\208', '\209', '\210',
                  '\211', '\212', '\213', '\214', '\215', '\216', '\217', '\218', '\219', '\220',
                  '\221', '\222', '\223', '\224', '\225', '\226', '\227', '\228', '\229', '\230',
                  '\231', '\232', '\233', '\234', '\235', '\236', '\237', '\238', '\239', '\240',
                  '\211', '\242', '\243', '\244', '\245', '\246', '\247', '\248', '\249', '\250',
                  '\251', '\252', '\253', '\254', '\255', '\256', '\257', '\258', '\259', '\260',
                  '\261', '\262', '\263', '\264', '\265', '\266', '\267', '\268', '\269', '\270',
                  '\271', '\272', '\273', '\274', '\275', '\276', '\277', '\278', '\279', '\280',
                  '\281', '\282', '\283', '\284', '\285', '\286', '\287', '\288', '\289', '\290',
                  '\291', '\292', '\293', '\294', '\295', '\296', '\297', '\298', '\299', '\300',
                  '\301', '\302', '\303', '\304', '\305', '\306', '\307', '\308', '\309', '\310',
                  '\311', '\312', '\313', '\314', '\315', '\316', '\317', '\318', '\319', '\320',
                  '\321', '\322', '\323', '\324', '\325', '\326', '\327', '\328', '\329', '\330',
                  '\331', '\332', '\333', '\334', '\335', '\336', '\337', '\338', '\339', '\340',
                  '\311', '\342', '\343', '\344', '\345', '\346', '\347', '\348', '\349', '\350',
                  '\351', '\352', '\353', '\354', '\355', '\356', '\357', '\358', '\359', '\360',
                  '\361', '\362', '\363', '\364', '\365', '\366', '\367', '\368', '\369', '\370',
                  '\371', '\372', '\373', '\374', '\375', '\376', '\377'};

bool isBufferInitialized = false;


tToken* getNextTokenFromScanner(){

    tToken *token = getNextToken();
    if (token == NULL){
        freeAndExit(INTERPRETER_ERROR);
    }
    if (token->state == s_lex_err){
        freeAndExit(LEXICAL_ERROR);
    }
    return token;
}

tToken* initToken(tToken *globalToken){

    ///TODO osetrit
    if ( (globalToken = (tToken*) malloc(sizeof(tToken))) == NULL) {
        return NULL;
    }

    if(( globalToken->data = (char*) malloc(sizeof(char))) == NULL){
        return NULL;
    }
    globalToken->data[0] = '\0';
    globalToken->num_of_chars = 0;
    globalToken->allocate_mem = 1;
    globalToken->state = s_start;

/*   printf("initToken prints: \n");
   printf("data  = %s \n", globalToken->data);
   printf("size  = %d \n", globalToken->size);
   printf("mem   = %d \n", globalToken->allocate_mem);
   printf("state = %d \n", globalToken->state);
   printf("--------------------------\n\n");*/

    return globalToken;
}

/*
 * add new character to token -> realocate structure + add new character
 */
tToken* editToken(tToken *globalToken, int character){

    if ((globalToken->data = (char *) realloc(globalToken->data, (size_t) globalToken->allocate_mem + 1)) != NULL) {
        // assign alocated memory into token
        globalToken->allocate_mem = 1 + globalToken->allocate_mem;
    }

    // on next position add character
    globalToken->data[globalToken->num_of_chars] = (char)character;
    // increment number of characters in token
    globalToken->num_of_chars++;
    // add EOS(end of string)
    globalToken->data[globalToken->num_of_chars] = '\0';


    /*printf("--------------------------\n");
    printf("editToken prints: \n");
    printf("data  = %s \n", globalToken->data);
    printf("size  = %d \n", globalToken->num_of_chars);
    printf("mem   = %d \n", globalToken->allocate_mem);
    printf("state = %d \n", globalToken->state);
    printf("--------------------------\n");*/


    //printf("allocate mem in editToken = %d\n", globalToken->allocate_mem);

    return globalToken;
}

void clearToken(tToken *globalToken) {
    globalToken->data[0] = '\0';
    globalToken->num_of_chars = 0;
}

//TODO: void clearToken()
//TODO: void changeTokenState(); maybe?


tToken* getNextToken() {
    int helper_var;
    int pom;
    int asci_to_int;
    char *helper_char;

    //pokial buffer nebol nainicializovany - ucinime tak
    if (!isBufferInitialized){
        initBuffer();
    }

    //najskor skontrolujeme ci je buffer tokenov prazdny - teda prva polozka ukazuje na NULL
    //pokial buffer nie je prazdny returneme ukazatel na token z bufferu inak pokracujeme v analyze
    if (tokensBuffer[0] != NULL) {
        //buffer je typu FIFO
        tToken *helper_token = tokensBuffer[0];

        //zaradom poposuvame vsetky tokeny v poli o jeden do lava, tym prvy vyhodime a posledny musime nastavit na NULL
        for (int i = 1; i < BUFFER_SIZE; i++) {
            tokensBuffer[i - 1] = tokensBuffer[i]; //posunieme polozku do lava
            tokensBuffer[i] = NULL; // zaroven ju nastavime na NULL - v dalsej iteracii sa prepise ale posledna iteracia zaisti aby tam neostala vysiet hodnota
        }

        return helper_token;
    }else {
        // define new pointer to struct tToken

        // init novy token
        if (globalToken == NULL) {
            globalToken = initToken(globalToken);
        }

#ifdef _DEBUG
        printf("inside function getNextToken\n");
#endif


        int state = s_start;
        int character;

        clearToken(globalToken);

        while (1) {
            if(file == NULL){
                freeAndExit(FILE_ERROR);
            }
            character = getc(file);

#ifdef _DEBUG
            printf("----------------\n");
            printf("original char  = '%c' \n", (char) character);
            printf("state pred switchom = %d \n", state);
#endif

            switch (state) {

                case s_start: {

                    /**
                     *  States go to end
                     */

                    if (isspace(character)) {
                        state = s_start;
                        break;
                    }
                    else if(isalpha(character) || character == '$' || character == '_') {
                        state = s_identifier;
                    }
                    else if (character == '+') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '+', state = s_end \n");
#endif
                        globalToken->state = s_plus;
                        state = s_plus;
                    }
                    else if (character == '-') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '-', state = s_end \n");
#endif
                        globalToken->state = s_minus;
                        state = s_minus;
                    }
                    else if (character == '*') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '*' \n");
#endif
                        globalToken->state = s_multiply;
                        state = s_multiply;
                    }
                    else if (character == '{') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '{' \n");
#endif
                        globalToken->state = s_left_vinculum;
                        state = s_left_vinculum;
                    }
                    else if (character == '}') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '}' \n");
#endif
                        globalToken->state = s_right_vinculum;
                        state = s_right_vinculum;
                    }
                    else if (character == '(') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '(' \n");
#endif
                        globalToken->state = s_left_bracket;
                        state = s_left_bracket;
                    }
                    else if (character == ')') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = ')' \n");
#endif
                        globalToken->state = s_right_bracket;
                        state = s_right_bracket;
                    }
                    else if (character == ';') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = ';' \n");
#endif
                        globalToken->state = s_semicolon;
                        state = s_semicolon;
                    }

                    else if (character == ',') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = ',' \n");
#endif
                        globalToken->state = s_comma;
                        state = s_comma;
                    }
                        // is it number start with 0 ?
                    else if (character == '0') {
#ifdef _DEBUG
                        printf("s_start nasli sme number = '0' \n");
#endif
                        state = s_helper_first_zero;

                    }
                    else if (isdigit(character) && character != '0') {
#ifdef _DEBUG
                        printf("s_start nasli sme number = '1-9' \n");
#endif
                        globalToken->state = s_helper_real_number;
                        state = s_helper_real_number;
                    }
                    else if (character == 34) {
#ifdef _DEBUG
                        printf("narazili sme na znak 'zlozena_uvodzovka' (zaciatok stringu) \n");
#endif
                        state = s_helper_string_start;
                    }
                    else if (character == EOF) {
#ifdef _DEBUG
                        printf("s_start narazili sme na EOF!\n");
#endif
                        globalToken->state = s_eof;
                        globalToken->data[0] = 'e';
                        globalToken->data[1] = 'o';
                        globalToken->data[2] = 'f';
                        state = s_end;
                        break;
                    }
                        /**
                         *
                         *  State go to the next state
                         *
                         */
                    else if (character == '|') {
#ifdef _DEBUG
                        printf("s_start sme nasli  znak '|' \n");
#endif
                        state = s_binary_or;
                    }
                    else if (character == '&') {
#ifdef _DEBUG
                        printf("s_start sme nasli  znak '&' \n");
#endif
                        state = s_binary_and  ;
                    }

                    else if (character == '<') {
#ifdef _DEBUG
                        printf("s_start sme nasli  znak '<' \n");
#endif
                        state = s_less;
                    }
                    else if (character == '>') {
#ifdef _DEBUG
                        printf("s_start sme nasli  znak '>' \n");
#endif
                        state = s_more;

                    }
                    else if (character == '/') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '*' \n");
#endif
                        state = s_helper_divide_or_comment;

                    }
                    else if (character == '!') {
#ifdef _DEBUG
                        printf("s_start nasli sme char = '!' \n");
#endif
                        state = s_binary_not;
                    }
                    else if (character == '=') {
                        state = s_equal_sign;
#ifdef _DEBUG
                        printf("s_start sme nasli znak = '=' \n");
#endif
                    }
                    else {
#ifdef _DEBUG
                        printf("s_lex_err ERR!\n");
#endif
                        state = s_lex_err;
                        break;
                    }
                    // edit finded token
                    if ((character != '0') && (state != s_helper_divide_or_comment)) {
                        editToken(globalToken, character);
                    }

                    break;
                }

                case s_identifier:
#ifdef _DEBUG
                    printf("znak vyhodnoteny ako jednoduchy identifikator\n");
#endif

                    // TODO: osetrit ak pride identifier prve cislo napr 1some_identifier = lex_error
                    if (isdigit(character) || isalnum(character) || character == '_' || character == '$') {
#ifdef _DEBUG
                        printf("edituje sa v simple_identifier\n");
#endif
                        state = s_identifier;
                        editToken(globalToken, character);
                    }
                    else if (character == '.') {
                        editToken(globalToken, character);
                        //ungetc(character, file);
                        state = s_helper_identifier_isSpace;
                    }
                    else {
                        // end of identifier

                        int same = 0;
                        int isBoolean = 0;

                        // check if identifier is between reserved words
                        for (int i = 0; i < NUM_OF_KEYWORDS; ++i) {
                            if (strcmp(globalToken->data, keyWords[i]) == 0) {
                                same = 1;
                            }
                        }
                        for (int i = 0; i < 2; ++i) {
                            if (strcmp(globalToken->data, boolValues[i]) == 0) {
                                isBoolean = 1;
                            }
                        }
                        if (same == 1) {
                            globalToken->state = s_keyword;
                            ungetc(character, file);
                            state = s_keyword;
                        }
                        else if(isBoolean == 1) {
                            globalToken->state = s_boolean;
                            ungetc(character, file);
                            state = s_boolean;
                        }
                        else {
                            globalToken->state = s_identifier;
                            ungetc(character, file);
                            state = s_end;
                        }
                    }
                    break;

                case s_helper_identifier_isSpace:

                    if(isspace(character)){
                        state = s_lex_err;
                    }
                    else if(isdigit(character) || isalnum(character) || character == '_' || character == '$'){
                        editToken(globalToken, character);
                        //ungetc(character, file);
                        state = s_helper_identifier;
                    }
                    else {
                        state = s_lex_err;
                    }
                    break;

                case s_helper_identifier:

                    globalToken->state = s_classified_identifier;

                    if(isdigit(character) || isalnum(character)){
                        editToken(globalToken, character);
                        //ungetc(character, file);
                        state = s_helper_identifier;
                    }
                    else {
                        ungetc(character, file);
                        state = s_end;
                    }

                    break;

                case s_comment_start:
#ifdef _DEBUG
                    printf("tento case ignoruje znaky v kommente\n");
#endif
                    globalToken->state = s_comment_start;
                    ungetc(character, file);
                    state = s_end;
                    break;

                case s_equal_sign:
#ifdef _DEBUG
                    printf("sme v stave s_equal sign\n");
#endif

                    if (character == '=') {
                        globalToken->state = s_equal_expression;
                        editToken(globalToken, character);
                        state = s_equal_expression;
                    }
                    else {
                        globalToken->state = s_equal_sign;
                        state = s_end;
                        ungetc(character, file);
                    }
                    break;

                case s_equal_expression:
#ifdef _DEBUG
                    printf("stav s_equal_expression teda -> '==' \n");
#endif
                    ungetc(character, file);
                    state = s_end;
                    break;

                case s_less:
#ifdef _DEBUG
                    printf("sme v stave s_less\n");
#endif
                    if (character == '=') {
                        state = s_less_equal;
                        globalToken->state = s_less_equal;
                        globalToken = editToken(globalToken, character);
#ifdef _DEBUG
                        printf("vyhodnotili sme ze dalsi znak za < je =\n");
#endif
                    }
                    else {
                        ungetc(character, file);
                        globalToken->state = s_less;
                        state = s_end;
                    }
                    break;

                case s_more:
#ifdef _DEBUG
                    printf("sme v stave s_more \n");
#endif
                    if (character == '=') {
                        state = s_more_equal;
                        globalToken->state = s_more_equal;
                        editToken(globalToken, character);
#ifdef _DEBUG
                        printf("vyhodnotili sme ze dalsi znak za > je =\n");
#endif
                    }
                    else {
                        ungetc(character, file);
                        globalToken->state = s_more;
                        state = s_end;
                    }
                    break;

                case s_less_equal:

                    state = s_end;

                    ungetc(character, file);
#ifdef _DEBUG
                printf("VYSLEDNY TOKEN JE <= yeaah!!!\n");
                    printf("--------------------------\n");
                    printf("s_less_equal prints: \n");
                    printf("data  = %s \n", globalToken->data);
                    printf("size  = %d \n", globalToken->num_of_chars);
                    printf("mem   = %d \n", globalToken->allocate_mem);
                    printf("state = %d \n", globalToken->state);
                    printf("--------------------------\n");
#endif
                    break;

                case s_more_equal:

                    state = s_end;
                    ungetc(character, file);
#ifdef _DEBUG
                    printf("VYSLEDNY TOKEN JE <= yeaah!!!\n");
#endif
                    break;

                case s_binary_not:
                    if (character == '=') {
                        globalToken->state = s_diff_expression;
                        editToken(globalToken, character);
                        state = s_end;
                    }
                    else {
#ifdef _DEBUG
                        printf("vratime state s_binary_not");
#endif
                        globalToken->state = s_binary_not;
                        ungetc(character, file);
                        state = s_end;
                    }
                    break;

                    /*
                     *
                     * States for detect type of number
                     *
                     */

                case s_helper_real_number:
                    if (isdigit(character)) {
                        editToken(globalToken, character);
                        state = s_helper_real_number;
                    }
                    else if (character == 'e' || character == 'E') {
                        editToken(globalToken, character);
                        globalToken->state = s_double;
                        state = s_helper_exp_char;
                    }
                    else if (character == '.') {
                        editToken(globalToken, character);
                        state = s_helper_double;
                    }
                    else if (isalpha(character)) {
                        state = s_lex_err;
                    }
                    else if (character == EOF || isspace(character) || character == '\n') {
                        state = s_int;
                    }
                    else {
                        // TODO here we cant give lex_err because other chars could be anything :(
                        ungetc(character, file);
                        state = s_int;
                    }
                    break;
                    // TODO is important to have this state with some changes?
                case s_helper_first_zero:

                    if (character == '0') {
                        state = s_helper_first_zero;
                    }
                    else if (character == 'e' || character == 'E') {
                        editToken(globalToken, character);
                        globalToken->state = s_double;
                        state = s_helper_exp_char;
                    }
                    else if (character == '.') {
                        editToken(globalToken, '0');
                        editToken(globalToken, character);
                        state = s_helper_double;
                    }
                    else if (isdigit(character) && character != 0) {
                        ungetc(character, file);
                        state = s_helper_real_number;
                    }
                        // TODO how to handle lex err ? idk
                    else {
                        editToken(globalToken, '0');
                        ungetc(character, file);
                        globalToken->state = s_int;
                        state = s_end;
                    }
                    break;

                case s_int:
#ifdef _DEBUG
                    printf("cislo je integer!!!!!\n");
#endif
                    globalToken->state = s_int;
                    ungetc(character, file);
                    state = s_end;
                    break;

                    // todo how to find difference between chars );( and &^ ????? omggg
                case s_helper_double:
                    if (isdigit(character)) {
                        editToken(globalToken, character);
                        state = s_helper_double;
                    }
                    else if (character == 'E' || character == 'e') {
                        editToken(globalToken, character);
                        state = s_helper_exp_char;
                    }
                    else if (isalpha(character)) {
                        state = s_lex_err;
                    }
                        // bug! SPACe is not lex erro cause 12.3 ; is good
                        // but 12. 3 is not good so idk
                        //else if (isspace(character)) {
                        /*state = s_double;
                        globalToken->state = s_double;
                        ungetc(character, file);*/
                        //state = s_lex_err;
                        //}
                        /*else if (character == ';') {
                            *//*state = s_double;
                        globalToken->state = s_double;
                        ungetc(character, file);*//*
                     *///   state = s_lex_err;
                        //}
                    else {
                        //state = s_lex_err;
                        state = s_double;
                        globalToken->state = s_double;
                        ungetc(character, file);
                    }
                    /*else {
                        ungetc(character, file);
                        state = s_lex_err;
                    }*/
                    break;

                case s_helper_exp_char:
                    if (isdigit(character) && character != '0') {
                        editToken(globalToken, character);
                        state = s_helper_zero_to_nine;
                    }
                    else if (character == '+' || character == '-') {
                        editToken(globalToken, character);
                        state = s_helper_exp_sign;
                    }
                    else if (character == '0') {
                        state = s_helper_exp_first_zero;
                    }
                    else if(isalpha(character)) {
                        state = s_lex_err;
                        ungetc(character, file);
                    }
                    else if (character == EOF || isspace(character) || character == '\n') {
                        state = s_lex_err;
                    }
                    else {
                        // lex_err
                        /*ungetc(character, file);
                        state = s_end;*/

                        state = s_lex_err;
                        //ungetc(character, file);
                    }

                    break;

                case s_helper_exp_first_zero:
                    if (character == '0') {
                        state = s_helper_exp_first_zero;
                    }
                    else if (isdigit(character) && character != '0') {
                        ungetc(character, file);
                        state = s_helper_zero_to_nine;
                    }
                    else {
                        ungetc(character, file);
                        state = s_lex_err;
                    }
                    break;

                case s_helper_zero_to_nine:
                    if (isdigit(character)) {
                        editToken(globalToken, character);
                        state = s_helper_zero_to_nine;
                    }
                    else if (character == EOF || isspace(character) || character == '\n') {
                        state = s_double;
                    }
                    else if (isalpha(character)){
                        state = s_lex_err;
                    }
                    else {
                        // lex err
                        /*ungetc(character, file);
                        state = s_lex_err;*/
                        state = s_double;
                        ungetc(character, file);
                    }
                    break;

                case s_helper_exp_sign:
#ifdef _DEBUG
                    printf("nasli sme +/- v exponente");
#endif
                    if (character == '0') {
                        state = s_helper_exp_first_zero;

                    }
                    else if (isdigit(character) && character != '0') {
                        state = s_helper_zero_to_nine;
                        ungetc(character, file);
                    }
                    else {
                        // lex err
                        ungetc(character, file);
                        state = s_lex_err;
                    }
                    break;

                case s_double:
                    globalToken->state = s_double;
                    ungetc(character, file);
                    state = s_end;

                    break;

                    /*
                     *
                     * States for detect correct string
                     *
                     */

                case s_helper_string_start:

                    clearToken(globalToken);

                    globalToken->state = s_string;
                    // 92 -> '\'
                    if(character == 92) {
                        state = s_special_character;
                    }
                    else if (character == 34){
                        state = s_end;
                    }
                    else if (isascii(character)) { //&& character != '0'
#ifdef _DEBUG
                        printf("toto je v datach po cisteni = %s \n", globalToken->data);
                        printf("TOOT JE CHATAKTEEER v stringu = %c \n", character);
                        printf("JETO ZNAK a neni to 0!!!!!!\n");
#endif
                        editToken(globalToken, character);
                        globalToken->state = s_string;
                        state = s_helper_string_one_to_nine;
                    }
                    else if (character == EOF){
                        state = s_lex_err;
                    }

                    else {
                        // lex_err
                        globalToken->state = s_lex_err;
                    }
                    break;

                case s_helper_string_one_to_nine:
                    // 92 = '\'
                    // 34 = "
                    // 39 = "
                    // 10 = newline
                    if (character == 10){
                        state = s_lex_err;
                    }
                    else if (character == 92){
                        state = s_special_character;
                    }
                    else if (isascii(character) && (character != 34)) {
#ifdef _DEBUG
                        printf("zapisujem char\n");
#endif
                        editToken(globalToken, character);
                        globalToken->state = s_string;
                        state = s_helper_string_one_to_nine;
                    }
                    else if(character == EOF){
                        state = s_lex_err;
                    }
                        /*else if (character == 92){
    #ifdef _DEBUG
                            printf("naasli sme backslash");
    #endif
                            state = s_end;
                        }*/
                    else if (character == 34) {
#ifdef _DEBUG
                        printf("narazili sme na zlozenu_uvodzovku");
#endif
                        state = s_helper_string_escape;
                    }
                    else {
                        // lex_err
                        globalToken->state = s_lex_err;
                        state = s_lex_err;
                    }
                    globalToken->state = s_string;
                    break;

                case s_helper_string_escape:
                    // end of escape sequence
                    /*if(character == '0'){
                        printf("nasli sme koniec escape sekvencie \n");
                        state = s_helper_string_escape;
                    }
                    else if (character == 34) {
                        printf("nasli sme znak konca stringu \n");
                        state = s_end;
                    }*/

                    // other character
                    ungetc(character, file);
                    state = s_end;


                    break;

                case s_helper_divide_or_comment:

                    if(character == '/'){
                        state = s_helper_simple_comment;
                    }
                    else if(character == '*'){
                        state = s_helper_block_comment_start;
                    }
                    else {
#ifdef _DEBUG
                        printf("nasli sme char = '/' je to delenie \n");
#endif
                        state = s_divide;
                        ungetc(character, file);
                        char slash = '/';
                        editToken(globalToken, slash);
                        globalToken->state = s_divide;
                    }

                    break;

                case s_helper_simple_comment:

                    if (character == '\n') {
#ifdef _DEBUG
                        printf("-----------------------------NASLI SME NEWLINE\n");
#endif
                        state = s_start;
                    }
                    else if (character == EOF ){
                        state = s_eof;
                    }
                    else {
                        state = s_helper_simple_comment;
                    }

                    break;

                case s_helper_block_comment_start:

                    if (character == '*'){
                        state = s_helper_block_comment_escape;

                    }
                    else if (character == EOF){
                        state = s_lex_err;
                    }
                    else {
                        state = s_helper_block_comment_start;
                    }

                    break;

                case s_helper_block_comment_escape:

                    if (character == '/') {
                        state = s_start;
                    }
                    else {
                        state = s_helper_block_comment_start;
                    }

                    break;

                case s_binary_and :


                    if(character == '&') {
                        editToken(globalToken, character);
                        globalToken->state = s_binary_and  ;
                        state = s_end;
                    }

                    break;

                case s_binary_or:


                    if(character == '|') {
                        editToken(globalToken, character);
                        globalToken->state = s_binary_or;
                        state = s_end;
                    }

                    break;

                case s_special_character:

#ifdef _DEBUG
                    printf("\n");
                    printf("\n");
                    printf("state s_special_character!!!!!!!!!! \n");
                    printf("v datach je = %s \n", globalToken->data);
#endif
                    //editToken(globalToken, '\\');


                    if(character == 'n') {
#ifdef _DEBUG
                        printf("supneme tam aj znak 'n' a pokracujeme v citani stringu \n");
#endif
                        //editToken(globalToken, character);
                        editToken(globalToken, '\n');
                        state = s_helper_string_one_to_nine;
                    }
                    else if (character == '"'){
#ifdef _DEBUG
                        printf(" supneme tam aj znak ZLOZENA UVODZOVKA a pokracujeme v citani stringu \n");
#endif
                        editToken(globalToken, '\"');
                        state = s_helper_string_one_to_nine;
                    }
                    else if (character == '\\'){
#ifdef _DEBUG
                        printf(" supneme tam aj znak backslash a pokracujeme v citani stringu \n");
#endif
                        editToken(globalToken, '\\');
                        state = s_helper_string_one_to_nine;
                    }
                    else if (character == 't'){
#ifdef _DEBUG
                        printf(" supneme tam aj znak backslash a pokracujeme v citani stringu \n");
#endif
                        editToken(globalToken, '\t');
                        state = s_helper_string_one_to_nine;
                    }
                    else if(character == '0' || character == '1' || character == '2' || character == '3') {

                        switch (character) {
                            case '0':
                                helper_char = "";
                                //helper_var = 0;
                                state = s_helper_spec_second_zero;

                                break;

                            case '1':
                                helper_var = 1;
                                state = s_helper_spec_one_two;

                                break;
                            case '2':
                                helper_var = 2;
                                state = s_helper_spec_one_two;

                                break;

                            case '3':
                                helper_var = 3;

                                state = s_helper_spec_three;
                                break;

                            default:
#ifdef _DEBUG
                                printf("padlo to v s_spec_char \n");
#endif
                                state = s_lex_err;
                        }
#ifdef _DEBUG
                        printf("helper var = %d \n", helper_var);

                        printf(" prvy ascii znak \n");
                        //editToken(globalToken, '\1');
#endif

                    }
                    else {
                        state = s_lex_err;
                    }
                    break;

                case s_helper_spec_second_zero:
#ifdef _DEBUG
                    printf("s_helper_spec_second_zero \n");
#endif
                    //ungetc(character, file);
                    asci_to_int = character - '0';

                    switch (character) {

                        case '0':
                            state = s_helper_spec_third_zero;

                            break;

                        case '1':
                            state = s_helper_spec_third_state;

                            break;
                        case '2':
                            state = s_helper_spec_third_state;

                            break;

                        case '3':
                            state = s_helper_spec_third_state;

                            break;
                        case '4':
                            state = s_helper_spec_third_state;

                            break;
                        case '5':
                            state = s_helper_spec_third_state;

                            break;
                        case '6':
                            state = s_helper_spec_third_state;

                            break;
                        case '7':
                            state = s_helper_spec_third_state;

                        case '8':
                            state = s_helper_spec_third_state;

                        case '9':
                            state = s_helper_spec_third_state;

                            break;
                        default:
                            state = s_lex_err;
                    }

                    helper_var =  concat(helper_var, asci_to_int);
#ifdef _DEBUG
                printf("\n\n---------------------------\n");
                    printf("je to cislo \n");
                    printf("nacitame dalsi znak po 0 \n");
                    printf("int before concat     =  %d \n", helper_var);
                    printf("int value from ascii  =  %d \n", asci_to_int);
#endif

#ifdef _DEBUG
                printf("konkatnute 1. a 2. num = %d \n", helper_var);
                    printf("---------------------------\n\n");
#endif

                    break;

                case s_helper_spec_third_zero:
#ifdef _DEBUG
                    printf("s_helper_spec_third_zero \n");
#endif
                    //ungetc(character, file);

                    asci_to_int = character - '0';

                    switch (character) {

                        case '0':
                            state = s_lex_err;

                            break;

                        case '1':
                            state = s_helper_spec_third_state;

                            break;
                        case '2':
                            state = s_helper_spec_third_state;

                            break;

                        case '3':
                            state = s_helper_spec_third_state;

                            break;
                        case '4':
                            state = s_helper_spec_third_state;

                            break;
                        case '5':
                            state = s_helper_spec_third_state;

                            break;
                        case '6':
                            state = s_helper_spec_third_state;

                            break;
                        case '7':
                            state = s_helper_spec_third_state;

                        case '8':
                            state = s_helper_spec_third_state;

                        case '9':
                            state = s_helper_spec_third_state;
                            break;
                        default:
                            state = s_lex_err;
                    }
                    ungetc(character, file);

                    break;

                case s_helper_spec_three:

                    asci_to_int = character - '0';

                    switch (character) {
                        case '0':
                            state = s_helper_spec_third_state;

                            break;

                        case '1':
                            state = s_helper_spec_third_state;

                            break;
                        case '2':
                            state = s_helper_spec_third_state;

                            break;

                        case '3':
                            state = s_helper_spec_third_state;

                            break;
                        case '4':
                            state = s_helper_spec_third_state;

                            break;
                        case '5':
                            state = s_helper_spec_third_state;

                            break;
                        case '6':
                            state = s_helper_spec_third_state;

                            break;
                        case '7':
                            state = s_helper_spec_seven;

                            break;
                        default:
                            state = s_lex_err;
                            break;
                    }


                    helper_var =  concat(helper_var, asci_to_int);
#ifdef _DEBUG
                printf("\n\n---------------------------\n");
                    printf("je to cislo \n");
                    printf("nacitame dalsi znak po 3 \n");
                    printf("int before concat     =  %d \n", helper_var);
                    printf("int value from ascii  =  %d \n", asci_to_int);
#endif

#ifdef _DEBUG
                printf("konkatnute 1. a 2. num = %d \n", helper_var);
                    printf("---------------------------\n\n");
#endif

                    break;

                case s_helper_spec_seven:

//                    asci_to_int = character - '0';

                    switch (character) {
                        case '0':
                            state = s_helper_spec_third_state;

                            break;

                        case '1':
                            state = s_helper_spec_third_state;

                            break;
                        case '2':
                            state = s_helper_spec_third_state;

                            break;

                        case '3':
                            state = s_helper_spec_third_state;

                            break;
                        case '4':
                            state = s_helper_spec_third_state;

                            break;
                        case '5':
                            state = s_helper_spec_third_state;

                            break;
                        case '6':
                            state = s_helper_spec_third_state;

                            break;
                        case '7':
                            state = s_helper_spec_third_state;

                            break;

                        default:
                            state = s_lex_err;
                            break;
                    }

                    ungetc(character, file);

/*                    helper_var =  concat(helper_var, asci_to_int);
#ifdef _DEBUG
                    printf("\n\n---------------------------\n");
                    printf("je to cislo \n");
                    printf("nacitame dalsi znak po 7 \n");
                    printf("int before concat     =  %d \n", helper_var);
                    printf("int value from ascii  =  %d \n", asci_to_int);
#endif

#ifdef _DEBUG
                    printf("konkatnute 1. a 2. num = %d \n", helper_var);
                    printf("---------------------------\n\n");
#endif*/

                    break;

                case s_helper_spec_one_two:

                    if(isdigit(character)){

                        asci_to_int = character - '0';

#ifdef _DEBUG
                        printf("---------------------------\n");
                        printf("je to cislo \n");
                        printf("nacitame dalsi znak po 1/2 \n");
                        printf("int before concat     =  %d \n", helper_var);
                        printf("int value from ascii  =  %d \n", asci_to_int);
#endif
                        helper_var =  concat(helper_var, asci_to_int);
#ifdef _DEBUG
                        printf("konkatnute 1. a 2 num = %d \n", helper_var);
                        printf("---------------------------\n");
#endif
                        state = s_helper_spec_third_state;

                    }
                    else {
                        state = s_lex_err;
                    }


                    break;

                case s_helper_spec_third_state:

                    if(isdigit(character)){

                        asci_to_int = character - '0';
#ifdef _DEBUG
                        printf("---------------------------\n");
                        printf("je to cislo \n");
                        printf("nacitame 3. cislo \n");
                        printf("int before concat     =  %d \n", helper_var);
                        printf("int value from ascii  =  %d \n", asci_to_int);
#endif
                        helper_var =  concat(helper_var, asci_to_int);

#ifdef _DEBUG
                        printf("konkatnute 1. a 2 num v 3. state!!!! = %d \n", helper_var);
                        printf("---------------------------\n");
#endif

                        editToken(globalToken,ascii[helper_var]);
#ifdef _DEBUG
                        printf("tokeeeeeeeen data final = %s \n", globalToken->data);
#endif
                        state = s_helper_string_one_to_nine;

                    }
                    else {
                        state = s_helper_string_one_to_nine;
                    }

                    break;

                case s_lex_err:
                    globalToken->state = s_lex_err;
                    return globalToken;

                case s_plus:
                case s_minus:
                case s_multiply:
                case s_eof:

                case s_divide:
                case s_left_vinculum:
                case s_right_vinculum:
                case s_left_bracket:
                case s_right_bracket:
                case s_semicolon:
                case s_comma:
                case s_keyword:
                case s_boolean:


                case s_end:
#ifdef _DEBUG
                    printf("------------------------------------\n");
                    printf("vratime token state =  %d \n", globalToken->state);
                    printf("vratime token data  =  %s \n", globalToken->data);
                    printf("return z funkcie getNextToken\n\n");
#endif
                    ungetc(character, file);

                    return globalToken;

                default:
#ifdef _DEBUG
                    printf("sme v defaulte\n");
#endif
                    globalToken->state = s_lex_err;
                    return globalToken;
            }
        }
    }
}





int concat(int x, int y){
    return  10*x + y;
}

void initBuffer(){
    //pokial buffer nebol nainicializovany - ucinime tak
    if (!isBufferInitialized){
        for(int i = 0; i<BUFFER_SIZE; i++){
            tokensBuffer[i] = NULL;
        }
        isBufferInitialized = true;
    }
}

// sluzi na vratenie tokenu do lexikalnej analyzi s tym ze ho iba ulozy do bufferu
int returnTokenAndSaveToBuffer(tToken * returned_token){

    //pokial buffer nebol nainicializovany - ucinime tak
    if (!isBufferInitialized){
        initBuffer();
    }

    //v cykle najdeme prvu polozku ktora ukazuje na NULL (teda je prazdna) a na tu ulozime ukazatel na token

    for(int i = 0; i< BUFFER_SIZE; i++){
        if (tokensBuffer[i] == NULL){
            tokensBuffer[i] = returned_token;
            return 0; //podarilo sa ulozit token vraciame navratovy typ ze vsetko prebehlo v poriadku

        }else if (i == BUFFER_SIZE - 1){ // pokial je buffer plny (ani posledna polozka sa nerovna NULL)
            return 100; // nepodarilo sa najst volnu polozku v buffry hrozilo by pretecenie, vraciame neuspesny return code
        }
    }
}

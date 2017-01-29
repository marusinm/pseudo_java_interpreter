/* ******************************** parser.h ******************************** */
/* Soubor:              parser.h                                              */
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
#include "scanner.h"
#include "common.h"


//#define _DEBUG_PARSER
//#define _DEBUG_BUFFER

#define STACK_SIZE 100

extern tToken *token;
extern t_hashtable *symbol_table;

/**
 * Rozparsuje klasifikovany identifikator a ulozy do pomocnej struktury tClassifiedIdentifier
 * zaroven nastavy tabulku symbolov pre danu triedu
 */
tClassifiedIdentifier * parseClassifiedIdentifier(tToken * token, tClassifiedIdentifier * classifiedIdentifier);


/**
 * definuje v ktorej faze dvojfazoveho prechodu
 * zdrojoveho kodu sa nachadzame, parser sa vola dva krat a
 * v kazdej faze urobi nieco ine
 */
typedef enum {
    first_phase = 1,
    second_phase = 2
} ParserPhase;

/**
 * struktura sluzi na docasne ulozenie lavej zlozenej zatvorky zlozeneho prikazu if, while
 * Zasobnik sa pouzije na kontrolu ci mozeme deklarovat premennu, ktoru v takomto
 * zlozenom prikaze deklarovat nemozeme. Deklarujeme iba ak je zasobnik prazdny
 */
typedef struct {
    tState * arr;                             /* pole pro uloĹľenĂ­ hodnot */
    int top;
} tCompoundStatementContextStack;

//operacie nad zasobnikom
void contextStackInit ( tCompoundStatementContextStack* s );
int contextStackEmpty ( const tCompoundStatementContextStack* s );
void contextStackPop ( tCompoundStatementContextStack* s );
void contextStackPush ( tCompoundStatementContextStack* s, tState state );
int contextStackFull(const tCompoundStatementContextStack* s);



ParserPhase parser_phase;
/**
 * funkcia rozparsuje zaciatok programu a rekurzivne rozparsuje triedy programu
 * zaroven sa vnara do parsovania nizsej urovne triedy
 *
 * @param phase -> definuje v ktorej z faz dvojfazoveho prechodu sa nachadzame
 */
int parseProgram();

/**
 * funkcia parsuje telo triedy
 */
int parseClassBody();

/**
 * parsovanie tela funkcie
 */
int parseFunctionBodyList();

/**
 * funkcia parsuje deklaraciu argumentov pri deklaracii funkcie
 */
int parseDeclarationOfFunctionArguments();

/**
 * funkcia naplni tabulku symbolov triedu ifj16 so vsetkymi potebnymi funkciami a ich parametrami
 */
void createIfj16();

/**
 * vytorenie funkcie print v tabulke symbolov pre triedu ifj16
 */
void createBuildInPrint(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie readInt v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadInt(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie readDouble v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadDouble(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie readString v tabulke symbolov pre triedu ifj16
 */
void createBuildInReadString(t_hashtable * ifj16_table);

/**
 * vygeneruje instrukciu SET pre pomocnu instrukcnu pasku funkcie (instrukcna paska pre parametre)
 */
void generateInstructionToHelperParamList(entry_s * dst, entry_s * src);

/**
 * vytorenie funkcie lenght v tabulke symbolov pre triedu ifj16
 */
void createBuildInLenght(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie sort v tabulke symbolov pre triedu ifj16
 */
void createBuildInSort(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie substr v tabulke symbolov pre triedu ifj16
 */
void createBuildInSubstr(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie compare v tabulke symbolov pre triedu ifj16
 */
void createBuildInCompare(t_hashtable * ifj16_table);

/**
 * vytorenie funkcie find v tabulke symbolov pre triedu ifj16
 */
void createBuildInFind(t_hashtable * ifj16_table);
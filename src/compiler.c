#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "compiler.h"
#include "memory.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
} Parser;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef void (*ParseFn)();

typedef struct {
    ParseFn prefix;
    ParseFn infix;
    Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compilingChunk;

static void expression();
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static void errorAt(Token* token, const char* message) {
    if(parser.panicMode) return;
    fprintf(stderr, "[Line %d] Error", token->line);
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {

    } else {
        fprintf(stderr, " at '%.*s'", token->length, token->start);
    }
    fprintf(stderr, ": %s\n", message);
    parser.hadError = true;
    parser.panicMode = true;
}

static void parserError(const char* message) {
    errorAt(&parser.current, message);
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}


static void advance() {
    parser.previous = parser.current;

    for(;;) {
        parser.current = scanToken();
        if(parser.current.type != TOKEN_ERROR) break;

        parserError(parser.current.start);
    }
}

// Checks if the token has the expected type
static void consume(TokenType type, const char* message) {
    if(parser.current.type == type) {
        advance();
        return;
    }

    parserError(message);
}

static Chunk* currentChunk() {
    return compilingChunk;
}

static void emitByte(uint8_t byte) {
    writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
    emitByte(byte1);
    emitByte(byte2);
}

static void endCompiler() {
    emitByte(OP_RETURN);

#ifdef DEBUG_PRINT_CODE
    if (!parser.hadError) {
        disassembleChunk(currentChunk(), "code");
    }
#endif

}

static void emitConstant(Value value) {
    writeConstant(currentChunk(), value, parser.previous.line);
}

static void number() {
    double value = strtod(parser.previous.start, NULL);
    emitConstant(NUMBER_VAL(value));
}

static void expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

// for parenthesis stuff and expressions
static void grouping() {
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expected ')' at the end of the expression");
}

static void unary() {
    TokenType operatorType = parser.previous.type;

    expression();
    switch (operatorType) {
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    default: 
        return;
    }
}

static void binary() {
    TokenType operatorType = parser.previous.type;
    ParseRule* rule = getRule(operatorType);
    parsePrecedence((Precedence)(rule->precedence + 1));
    switch (operatorType) {
        case TOKEN_PLUS: emitByte(OP_ADD); break;
        case TOKEN_MINUS: emitByte(OP_SUBTRACT); break;
        case TOKEN_SLASH: emitByte(OP_DIVIDE); break;
        case TOKEN_STAR: emitByte(OP_MULTIPLY); break;
        case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
        case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
        case TOKEN_GREATER:       emitByte(OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
        case TOKEN_LESS:          emitByte(OP_LESS); break;
        case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
        default: break;
    }
}

static void literal() {
    switch (parser.previous.type) {
        case TOKEN_TRUE: emitByte(OP_TRUE); break;
        case TOKEN_FALSE: emitByte(OP_FALSE); break;
        case TOKEN_NIL: emitByte(OP_NIL); break;
        default: return;
    }
}

static void string() {
    emitConstant(OBJ_VAL(copyString(parser.previous.start + 1, parser.previous.length - 2)));    
}

// Parser rules
ParseRule rules[] = {
    [TOKEN_LEFT_PAREN]    = {grouping, NULL,   PREC_NONE},
    [TOKEN_RIGHT_PAREN]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LEFT_BRACE]    = {NULL,     NULL,   PREC_NONE}, 
    [TOKEN_RIGHT_BRACE]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_COMMA]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_DOT]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_MINUS]         = {unary,    binary, PREC_TERM},
    [TOKEN_PLUS]          = {NULL,     binary, PREC_TERM},
    [TOKEN_SEMICOLON]     = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SLASH]         = {NULL,     binary, PREC_FACTOR},
    [TOKEN_STAR]          = {NULL,     binary, PREC_FACTOR},
    [TOKEN_BANG]          = {unary,    NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     binary, PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,     binary, PREC_COMPARISON},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {string,   NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {literal,  NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {literal,  NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {literal,  NULL,   PREC_NONE},
    [TOKEN_VAR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_WHILE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ERROR]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EOF]           = {NULL,     NULL,   PREC_NONE},
};

static ParseRule* getRule(TokenType type) {
    return &rules[type];
}

static void parsePrecedence(Precedence precedence) {
    // implement the precendence order for evaluating expressions;
    advance();
    ParseFn prefixRule = getRule(parser.previous.type)->prefix;
    if(prefixRule == NULL) {
        error("Expected an expression");
    }

    prefixRule();

    while(precedence <= getRule(parser.current.type)->precedence) {
        advance();
        ParseFn infixRule = getRule(parser.previous.type)->infix;
        infixRule();
    }
}

static void printStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ; after all statements");
    emitByte(OP_PRINT);
}

static bool check(TokenType token) {
    return parser.current.type == token;
}

static bool match(TokenType token) {
    if(!check(token)) return false;
    advance();
    return true;
}

static void expressionStatement() {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ; after every expression");
    emitByte(OP_POP);
}

static void statement() {
    if(match(TOKEN_PRINT)) {
        printStatement();
    } else {
        expressionStatement();
    }
}

static void synchronize() {
    parser.panicMode = false;
    while (parser.current.type != TOKEN_EOF) {
        if (parser.previous.type == TOKEN_SEMICOLON) return;
        switch (parser.current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
            return;

            default:
            ;
        }

        advance();
    }
}

static uint8_t identifierConstant(Token* token) {
    return makeConstant(OBJ_VAL(copyString(token->start, token->length)));
}

static uint8_t parseVariable(const char* errorMsg) {
    consume(TOKEN_IDENTIFIER, errorMsg);
    return identifierConstant(&parser.previous);
}

static void defineVariable(uint8_t global) {
    emitBytes(OP_DEFINE_GLOBAL, global);
}

static void varDeclaration() {
    uint8_t global = parseVariable("Expected a variable name");

    if(match(TOKEN_EQUAL)) {
        expression();
    } else {
        emitByte(OP_NIL);
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

    defineVariable(global);
}

static void declaration() {
    if(match(TOKEN_VAR)) {
        varDeclaration();
    } else {
        statement();
    }

    if(parser.panicMode) synchronize();
}

bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;
    compilingChunk = chunk;
    advance();
    
    while(!match(TOKEN_EOF)) {
        declaration();
    }

    endCompiler();
}
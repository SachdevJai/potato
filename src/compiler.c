#include <stdio.h>
#include <stdlib.h>


#include "debug.h"

#include "compiler.h"
#include "scanner.h"

Parser parser;
Chunk* compilingChunk;

static void expression();
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
    emitConstant(value);
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
        default:break;
    }
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
    [TOKEN_BANG]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_BANG_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER]       = {NULL,     NULL,   PREC_NONE},
    [TOKEN_GREATER_EQUAL] = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_LESS_EQUAL]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IDENTIFIER]    = {NULL,     NULL,   PREC_NONE},
    [TOKEN_STRING]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NUMBER]        = {number,   NULL,   PREC_NONE},
    [TOKEN_AND]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_CLASS]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_ELSE]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FALSE]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FOR]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_FUN]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_IF]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_NIL]           = {NULL,     NULL,   PREC_NONE},
    [TOKEN_OR]            = {NULL,     NULL,   PREC_NONE},
    [TOKEN_PRINT]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_RETURN]        = {NULL,     NULL,   PREC_NONE},
    [TOKEN_SUPER]         = {NULL,     NULL,   PREC_NONE},
    [TOKEN_THIS]          = {NULL,     NULL,   PREC_NONE},
    [TOKEN_TRUE]          = {NULL,     NULL,   PREC_NONE},
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


bool compile(const char* source, Chunk* chunk) {
    initScanner(source);
    parser.hadError = false;
    parser.panicMode = false;
    compilingChunk = chunk;
    advance();
    expression();
    consume(TOKEN_EOF, "Expect End of File Expression");
    endCompiler();
}
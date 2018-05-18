// -------------------------
// Generic Grafcet Framework - Preprocessor
// -------------------------

// MIT License:
//
// Copyright 2018 Gonçalo Santos
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "stretchy_buffer.h"

#define ArrayCount(arr) ((sizeof(arr))/sizeof(*arr))

static char *readEntireFileIntoMemoryAndNullTerminate(char *FileName) {
    char *Result = 0;

    FILE *File = fopen(FileName, "r");
    if(File)
    {
        fseek(File, 0, SEEK_END);
        size_t FileSize = ftell(File);
        fseek(File, 0, SEEK_SET);

        Result = (char *)malloc(FileSize + 1);
        fread(Result, FileSize, 1, File);
        Result[FileSize] = 0;

        fclose(File);
    }

    return(Result);
}

typedef enum {
    Token_Unknown,

    Token_Asterisk,
    Token_CloseBrace,
    Token_CloseBracket,
    Token_CloseParen,
    Token_Colon,
    Token_Comma,
    Token_OpenBrace,
    Token_OpenBracket,
    Token_OpenParen,
    Token_Semicolon,

    Token_String,
    Token_Identifier,

    Token_EndOfStream,
} token_type;

typedef struct {
    token_type Type;

    size_t TextLength;
    char *Text;
} token;

typedef struct {
    char *At;
} tokenizer;

bool isEndOfLine(char C) {
    bool Result = ((C == '\n') ||
                   (C == '\r'));

    return(Result);
}

bool isWhitespace(char C) {
    bool Result = ((C == ' ') ||
                   (C == '\t') ||
                   (C == '\v') ||
                   (C == '\f') ||
                   isEndOfLine(C));

    return(Result);
}

bool isAlpha(char C) {
    bool Result = (((C >= 'a') && (C <= 'z')) ||
                   ((C >= 'A') && (C <= 'Z')));

    return(Result);
}

bool isNumber(char C) {
    bool Result = ((C >= '0') && (C <= '9'));

    return(Result);
}

bool tokenEquals(token Token, char *Match) {
    char *At = Match;
    for(int Index = 0;
        Index < Token.TextLength;
        ++Index, ++At)
    {
        if((*At == 0) ||
           (Token.Text[Index] != *At))
        {
            return(false);
        }

    }

    bool Result = (*At == 0);
    return(Result);
}

static void eatAllWhitespace(tokenizer *Tokenizer) {
    for(;;)
    {
        if(isWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !isEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }

            if(Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

static token getToken(tokenizer *Tokenizer) {
    eatAllWhitespace(Tokenizer);

    token Token = {};
    Token.TextLength = 1;
    Token.Text = Tokenizer->At;
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': {Token.Type = Token_EndOfStream;} break;

        case '(': {Token.Type = Token_OpenParen;} break;
        case ')': {Token.Type = Token_CloseParen;} break;
        case ':': {Token.Type = Token_Colon;} break;
        case ',': {Token.Type = Token_Comma;} break;
        case ';': {Token.Type = Token_Semicolon;} break;
        case '*': {Token.Type = Token_Asterisk;} break;
        case '[': {Token.Type = Token_OpenBracket;} break;
        case ']': {Token.Type = Token_CloseBracket;} break;
        case '{': {Token.Type = Token_OpenBrace;} break;
        case '}': {Token.Type = Token_CloseBrace;} break;

        case '"':
        {
            Token.Type = Token_String;

            Token.Text = Tokenizer->At;

            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                   Tokenizer->At[1])
                {
                    ++Tokenizer->At;
                }
                ++Tokenizer->At;
            }

            Token.TextLength = Tokenizer->At - Token.Text;
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;

        default:
        {
            if(isAlpha(C))
            {
                Token.Type = Token_Identifier;

                while(isAlpha(Tokenizer->At[0]) ||
                      isNumber(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    ++Tokenizer->At;
                }

                Token.TextLength = Tokenizer->At - Token.Text;
            }

            else
            {
                Token.Type = Token_Unknown;
            }
        } break;
    }

    return(Token);
}

static bool requireToken(tokenizer *Tokenizer, token_type DesiredType)
{
    token Token = getToken(Tokenizer);
    bool Result = (Token.Type == DesiredType);
    return(Result);
}

void parseParentheses(tokenizer *Tokenizer) {
    for(;;) {
        token Token = getToken(Tokenizer);
        switch(Token.Type) {
            case Token_EndOfStream:
            case Token_CloseParen:
            {
                return;
            } break;

            case Token_OpenParen:
            {
                parseParentheses(Tokenizer);
            } break;
        }
    }
}

/* NOTE(nox): This is not supported by the compilers, so may result in different things...
 * If in the middle of the macro, it is better to encapsulate everything in parentheses.
 * If at the end, use __VA_ARGS__. */
void parseBraces(tokenizer *Tokenizer) {
    for(;;) {
        token Token = getToken(Tokenizer);
        switch(Token.Type) {
            case Token_EndOfStream:
            case Token_CloseBrace:
            {
                return;
            } break;

            case Token_OpenBrace:
            {
                parseBraces(Tokenizer);
            } break;
        }
    }
}

static char **States = 0;
static char **Transitions = 0;

typedef struct {
    char *Start, *End;
} argument;

typedef enum {
    Function_NewState,
    Function_NewTransition,
} function_type;

void parseFunction(tokenizer *Tokenizer, function_type Type) {
    if(requireToken(Tokenizer, Token_OpenParen)) {
        int NumberOfArguments = 0;
        argument Arguments[10] = {};
        bool Parsing = true;
        while(Parsing) {
            assert(NumberOfArguments < ArrayCount(Arguments));
            token Token = getToken(Tokenizer);

            if(!Arguments[NumberOfArguments].Start) {
                Arguments[NumberOfArguments].Start = Token.Text;
            }

            switch(Token.Type) {
                case Token_EndOfStream:
                {
                    return;
                } break;

                case Token_Comma:
                {
                    Arguments[NumberOfArguments++].End = Token.Text;
                    break;
                } break;

                case Token_OpenParen:
                {
                    parseParentheses(Tokenizer);
                } break;

                case Token_OpenBrace:
                {
                    parseBraces(Tokenizer);
                } break;

                case Token_CloseParen:
                {
                    Arguments[NumberOfArguments++].End = Token.Text;
                    Parsing = false;
                } break;
            }
        }

        switch(Type) {
            case Function_NewState:
            {
                if(NumberOfArguments == 3) {
                    enum { OutputIndex = 2 };
                    char *Name = calloc(1, Arguments[1].End - Arguments[1].Start + 1);
                    sprintf(Name, "X%.*s", Arguments[1].End - Arguments[1].Start, Arguments[1].Start);
                    sb_push(States, Name);

                    printf("STATE_OUTPUT_FUNCTION(stateAction_%s) %.*s\n",
                           Name,
                           Arguments[OutputIndex].End - Arguments[OutputIndex].Start,
                           Arguments[OutputIndex].Start);
                } else {
                    fprintf(stderr, "Syntax error: Incorrect number of arguments to newState.\n");
                }
            } break;

            case Function_NewTransition:
            {
                if(NumberOfArguments == 5) {
                    char *Name = calloc(1, Arguments[1].End - Arguments[1].Start + 1);
                    sprintf(Name, "%.*s", Arguments[1].End - Arguments[1].Start, Arguments[1].Start);
                    sb_push(Transitions, Name);

                    printf("TRANSITION_CONDITION_FUNCTION(transitionCondition_%s) { return (%.*s); }\n",
                           Name,
                           Arguments[4].End - Arguments[4].Start, Arguments[4].Start);
                } else {
                    fprintf(stderr, "Syntax error: Incorrect number of arguments to newTransition.\n");
                }
            } break;
        }
    } else {
        fprintf(stderr, "Syntax error: Missing parentheses.\n");
    }
}

int main(int ArgCount, char **Args) {
    if(ArgCount < 2) {
        return -1;
    }

    char *FileContents = readEntireFileIntoMemoryAndNullTerminate(Args[1]);
    tokenizer Tokenizer = {};
    Tokenizer.At = FileContents;

    printf("#if defined(OUTPUTS_AND_CONDITIONS)\n\n");
    bool Parsing = true;
    token PreviousToken = {};
    while(Parsing)
    {
        token Token = getToken(&Tokenizer);
        switch(Token.Type)
        {
            case Token_EndOfStream:
            {
                Parsing = false;
            } break;

            case Token_Identifier:
            {
                if(!tokenEquals(PreviousToken, "define")) {
                    if(tokenEquals(Token, "newState")) {
                        parseFunction(&Tokenizer, Function_NewState);
                    } else if(tokenEquals(Token, "newTransition")) {
                        parseFunction(&Tokenizer, Function_NewTransition);
                    }
                }
            } break;

            default:
            {
            } break;
        }
        PreviousToken = Token;
    }
    printf("\n#else\n\n");

    printf("typedef enum {\n");
    for(int I = 0; I < sb_count(States); ++I) {
        printf("    State_%s,\n", States[I]);
    }
    printf("    StateCount\n} state_id;\n");

    printf("\ntypedef enum {\n");
    for(int I = 0; I < sb_count(Transitions); ++I) {
        printf("    Transition_%s,\n", Transitions[I]);
    }
    printf("    TransitionCount\n} transition_id;\n");

    printf("\n#endif\n");
}

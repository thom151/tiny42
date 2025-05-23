#include "lexer.h"
#include "token.h"
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <algorithm>
#include "emitter.h"

class Parser {
    Lexer mLexer;
    Emitter *mEmitter;
    Token mCurrToken{};
    Token mPeekToken{};
    std::set<std::string> mSymbols{};
    std::set<std::string> mLabelsDeclared{};
    std::set<std::string> mLabelGotoed{};
    std::set<std::string> mVarStrings{};
    std::set<std::string> mVarNums{};

public:

    Parser ( Lexer lexer, Emitter *emitter)
    : mLexer{lexer}, mEmitter{emitter} {
            nextToken();
            nextToken();
    }

    //check if the expected token (argument) is the current Token
    bool checkToken( TokenType kind) {
        return kind == mCurrToken.getKind();
    }

    bool checkPeek( TokenType kind) {
        return kind == mPeekToken.getKind();
    }

    void nextToken() {
        mCurrToken = mPeekToken;
        mPeekToken = mLexer.getToken();
    }

    void match(TokenType kind) {
        if (!checkToken(kind)) {
            abort("Expected " + getKindName(kind) + ", got " + getKindName(mCurrToken.getKind()));
        }
        nextToken();
    }

    void abort(std::string message) {
        std::cerr << "Parsing error: " << message << '\n';
        std::exit(1);
    }


    void program() {
        std::cout<<"PROGRAM\n";
        mEmitter->headerLine("#include <iostream>");
        mEmitter->headerLine("int main() {");

        while (checkToken(NEWLINE)) {
            nextToken();

        }
        while (!checkToken(ENDOFFILE)) {
            statement();
        }

        mEmitter->emitLine("return 0;");
        mEmitter->emitLine("}");
    }

    void statement() {
        if (checkToken(PRINT)) {
            std::cout <<"STATEMENT-PRINT"<<'\n';
            nextToken();
        

            if (checkToken(STRING)) {
                //here is the string token
                std::string code = "std::cout<<\"" + mCurrToken.getText() + "\";";
                mEmitter->emitLine(code);
                nextToken();

                while (checkToken(NL)) {
                    mEmitter->emitLine("std::cout<<\"\\n\";");
                    nextToken();
                }
            } else if (checkToken(NL)) {
                mEmitter->emitLine("std::cout<<\"\\n\";");
                nextToken();
                while (checkToken(NL)) {
                     mEmitter->emitLine("std::cout<<\"\\n\";");
                     nextToken();    
                }
            }else {    
                //this is a number expression is anumber
                std::string code = "std::cout<<";
                mEmitter->emit(code);
                 expression();
                //close and then emit line
                mEmitter->emitLine(";");
 
             }

            while (checkToken(NL)) {
                mEmitter->emitLine("std::cout<<\"\\n\";");
                nextToken();    
            }
    
            
        } else if (checkToken(IF)) {
            std::cout<<"STATEMENT-IF\n";
            mEmitter->emit("if (");
            nextToken();//get the next token
            comparison();//make sure that the next token can be parsed by the comparison
            match(THEN);
            nl();
            mEmitter->emitLine("){");

            while (!checkToken(ENDIF)){
                statement();
            }

            match(ENDIF);

            mEmitter->emit("}");



        } else if(checkToken(WHILE)) {
            std::cout<<"STATEMENT-WHILE\n";
            mEmitter->emit("while(");
            nextToken();
            comparison();
            match(REPEAT);
            nl();
            mEmitter->emitLine("){");
            while(!checkToken(ENDWHILE)) {
                statement();
            }
            match(ENDWHILE);
            mEmitter->emitLine("}");



        } else if (checkToken(LABEL)) {
            std::cout<<"STATEMENT-LABEL\n";
            nextToken(); 
            //if label exsits, return error
            if (mLabelsDeclared.find(mCurrToken.getText())!= mLabelsDeclared.end()) {
                abort("label current exists" + mCurrToken.getText());
            }

            mLabelsDeclared.insert(mCurrToken.getText());
            match(IDENT);    
            mEmitter->emitLine(mCurrToken.getText() + ":");



        } else if (checkToken(GOTO)) {
            std::cout<<"STATEMENT-GOTO\n";
            //inset labels (currToken) in the mLabelsGotoed (can occur multiple times)
            nextToken(); 
            mLabelGotoed.insert(mCurrToken.getText());
            match(IDENT);   
            mEmitter->emitLine("goto " + mCurrToken.getText() + ";");



        } else if (checkToken(LET)) {
            std::cout<<"STATEMENT-LET\n";
            nextToken();
            
            if (checkToken(STR)) {
                nextToken();
                if (mVarStrings.find(mCurrToken.getText()) == mVarStrings.end()) {
                    std::cout<<"STR\n"; 
                    mVarStrings.insert(mCurrToken.getText());
    
                    for (std::string strs : mVarStrings) {
                        std::cout<<"VARIABLE: " <<strs<<'\n';
                     }
                    mEmitter->headerLine("std::string " + mCurrToken.getText() + ";");
                }

            } else if (checkToken(NUM)) {
                nextToken();
                std::cout<<"NUM\n";
                if (mVarNums.find(mCurrToken.getText()) == mVarNums.end()) { 
                    mVarNums.insert(mCurrToken.getText());
                    mEmitter->headerLine("float " + mCurrToken.getText() + ";");
                }

            }


            //meaning it's trying to declare without using STR or NUM
            if (mVarNums.find(mCurrToken.getText()) == mVarNums.end() &&
                mVarStrings.find(mCurrToken.getText()) == mVarStrings.end()) {
                abort("trying to declare a variable without type: " + mCurrToken.getText());
            }

            if (std::count(mVarNums.begin(), mVarNums.end(), mCurrToken.getText()) > 0) {
                mEmitter->emit(mCurrToken.getText() + "=");
                match(IDENT);
                match(EQ);
                expression();

                mEmitter->emitLine(";");
            } else if (std::count(mVarStrings.begin(), mVarStrings.end(), mCurrToken.getText()) > 0) {
                mEmitter->emit(mCurrToken.getText() + "=");
    
                match(IDENT);
                match(EQ);


                if(checkToken(STRING)) {
                    mEmitter->emitLine( "\"" + mCurrToken.getText() + "\";");
                    nextToken();
                } else {
                    abort("expected a string at: " + mCurrToken.getText());
                }
            }
            

        } else if (checkToken(INPUT)) {
            std::cout<<"STATEMENT-INPUT\n";
            nextToken();

               if (checkToken(STR)) {
                nextToken();
                if (mVarStrings.find(mCurrToken.getText()) == mVarStrings.end()) {
                    std::cout<<"STR\n"; 
                    mVarStrings.insert(mCurrToken.getText());    
                    mEmitter->headerLine("std::string " + mCurrToken.getText() + ";");
                }

                } else if (checkToken(NUM)) {
                    nextToken();
                    std::cout<<"NUM\n";
                    if (mVarNums.find(mCurrToken.getText()) == mVarNums.end()) { 
                        mVarNums.insert(mCurrToken.getText());
                        mEmitter->headerLine("float " + mCurrToken.getText() + ";");
                    }
                }


                //meaning it's trying to declare without using STR or NUM
                if (mVarNums.find(mCurrToken.getText()) == mVarNums.end() &&
                    mVarStrings.find(mCurrToken.getText()) == mVarStrings.end()) {
                    abort("trying to declare a variable without type: " + mCurrToken.getText());
                }

                mEmitter->emitLine("std::cin >> " + mCurrToken.getText() + ";");
                match(IDENT);  
            } else {
                abort("invalid statement at " + mCurrToken.getText());
            }
            nl();
    }

    void expression() {
        std::cout<<"EXPRESSION\n";
        term(); // 1
        while(checkToken(PLUS) ||checkToken( MINUS)) {
            mEmitter->emit(mCurrToken.getText());
            nextToken();
            term();
        }
        
        return;
    }

    void term() {
        std::cout<<"TERM\n";
        unary(); 

        while (checkToken(ASTERISK) || checkToken(SLASH)) {
            mEmitter->emit(mCurrToken.getText());
            nextToken();
            unary(); // 2
            
        }
    }

    void unary() {
        std::cout<<"UNARY\n"; 

        if (checkToken(PLUS) || checkToken(MINUS)) {
            mEmitter->emit(mCurrToken.getText());
            nextToken();
        }

        primary(); 
    }

    void primary() {
        std::cout<<"PRIMARY (" << mCurrToken.getText() << ")\n"; 
        
        if (checkToken(NUMBER)) {
            mEmitter->emit(mCurrToken.getText());
            nextToken();
        } else if (checkToken(IDENT)) {
            if(mVarNums.find(mCurrToken.getText()) == mVarNums.end() &&
                mVarStrings.find(mCurrToken.getText()) == mVarStrings.end()) {
                abort("variable has not been declared: " + mCurrToken.getText());
            }
            mEmitter->emit(mCurrToken.getText());
            nextToken();
        } else {
            abort("unexpected token" + mCurrToken.getText());
        }
    }
    
    void comparison() {
        std::cout<<"COMPARISON\n";
        expression();

        if (isComparisonOperator()) {

            //this just emits "<, > , = ,!= etc"
            mEmitter->emit(mCurrToken.getText());
            nextToken();
            expression();
        } else {
            abort("expected comparison operator");
        }

        while (isComparisonOperator()) {
            mEmitter->emit(mCurrToken.getText());
            nextToken();
            expression();
        }
        return;
    }

    bool isComparisonOperator() {
        return  checkToken(GT)   ||
                checkToken(GTEQ) ||
                checkToken(LT)   ||
                checkToken(LTEQ) ||
                checkToken(EQEQ) ||
                checkToken(NOTEQ);
    }

    void nl() {
        std::cout << "NEWLINE" << '\n';

        match(NEWLINE);
        while (checkToken(NEWLINE)) {
            nextToken();
        }
    }
};

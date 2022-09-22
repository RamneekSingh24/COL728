%require "3.2"

%code requires {
	#include <iostream>
	#include "ast.h"
}


%{
#include <cstdio>
#include <iostream>
#include "ast.h"
using namespace std;

yyTU* topLevelTU = new yyTU();

// stuff from flex that bison needs to know about:
extern "C" int yylex();
int yyparse();
extern "C" FILE *yyin;
 
void yyerror(const char *s);
%}

%union {
  int int_val;
  float float_val;
  char* str_val;   // "string type has a non-trivial copy constructor"
  yyAST* ast_node;
  UnaryOp un_op;
}

%type <ast_node> translation_unit
%type <ast_node> external_declaration
%type <ast_node> function_definition
%type <ast_node> declaration_specifiers declaration type_specifier direct_declarator
%type <ast_node> parameter_list parameter_declaration declarator parameter_type_list
%type <ast_node> compound_statement block_item_list block_item statement
%type <ast_node> labeled_statement expression_statement selection_statement iteration_statement jump_statement
%type <ast_node> expression assignment_expression conditional_expression
%type <ast_node> logical_or_expression logical_and_expression inclusive_or_expression  exclusive_or_expression
%type <ast_node> and_expression equality_expression relational_expression shift_expression additive_expression
%type <ast_node> multiplicative_expression cast_expression unary_expression postfix_expression primary_expression
%type <un_op>    unary_operator
%type <ast_node> argument_expression_list constant


%token	<str_val> IDENTIFIER STRING_LITERAL FUNC_NAME
%token  <int_val> I_CONSTANT
%token  <float_val> F_CONSTANT
%token  SIZEOF
%token	PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token	AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token	SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token	XOR_ASSIGN OR_ASSIGN
%token	TYPEDEF_NAME ENUMERATION_CONSTANT

%token	TYPEDEF EXTERN STATIC AUTO REGISTER INLINE
%token	CONST RESTRICT VOLATILE
%token	BOOL CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token	COMPLEX IMAGINARY 
%token	STRUCT UNION ENUM ELLIPSIS

%token	CASE DEFAULT IF ELSE SWITCH WHILE DO FOR GOTO CONTINUE BREAK RETURN

%token	ALIGNAS ALIGNOF ATOMIC GENERIC NORETURN STATIC_ASSERT THREAD_LOCAL

%start translation_unit
%%



primary_expression
	: IDENTIFIER {$$ = new yyIdentifier($1);}
	| constant   {$$ = $1;}
	| string
	| '(' expression ')' {$$ = $2;}
	| generic_selection
	;

constant
	: I_CONSTANT	{$$ = new yyIntegerLiteral($1);}	    /* includes character_constant */
	| F_CONSTANT    {$$ = new yyFloatLiteral($1);}
	| ENUMERATION_CONSTANT	/* after it has been defined as such */
	;

enumeration_constant		/* before it has been defined as such */
	: IDENTIFIER
	;

string
	: STRING_LITERAL
	| FUNC_NAME
	;

generic_selection
	: GENERIC '(' assignment_expression ',' generic_assoc_list ')'
	;

generic_assoc_list
	: generic_association
	| generic_assoc_list ',' generic_association
	;

generic_association
	: type_name ':' assignment_expression
	| DEFAULT ':' assignment_expression
	;

postfix_expression
	: primary_expression  {$$ = $1;}
	| postfix_expression '[' expression ']'
	| postfix_expression '(' ')'
	{
	    $$ = new yyBinaryOp($1, BinaryOp::FUNC_CALL, new yyArgumentExpressionList());
	}
	| postfix_expression '(' argument_expression_list ')'
	{
	    $$ = new yyBinaryOp($1, BinaryOp::FUNC_CALL, $3);
	}
	| postfix_expression '.' IDENTIFIER
	| postfix_expression PTR_OP IDENTIFIER
	| postfix_expression INC_OP
    {
        $$ = new yyUnaryOp(UnaryOp::POST_INC, $1);
    }
	| postfix_expression DEC_OP
    {
        $$ = new yyUnaryOp(UnaryOp::POST_DEC, $1);
    }
	| '(' type_name ')' '{' initializer_list '}'
	| '(' type_name ')' '{' initializer_list ',' '}'
	;

argument_expression_list
	: assignment_expression
	| argument_expression_list ',' assignment_expression
	;

unary_expression
	: postfix_expression {$$ = $1;}
	| INC_OP unary_expression
{
    $$ = new yyUnaryOp(UnaryOp::PRE_INC, $2);
}
	| DEC_OP unary_expression
{
    $$ = new yyUnaryOp(UnaryOp::PRE_DEC, $2);
}
	| unary_operator cast_expression
{
    $$ = new yyUnaryOp($1, $2);
}
	| SIZEOF unary_expression
	| SIZEOF '(' type_name ')'
	| ALIGNOF '(' type_name ')'
	;

unary_operator
	: '&'
	| '*'
	| '+' {$$ = UnaryOp::PL;}
	| '-' {$$ = UnaryOp::NEG;}
	| '~' {$$ = UnaryOp::NOT;}
	| '!' {$$ = UnaryOp::LOGICAL_NOT;}
	;

cast_expression
	: unary_expression {$$ = $1;}
	| '(' type_name ')' cast_expression
	;

multiplicative_expression
	: cast_expression {$$ = $1;}
	| multiplicative_expression '*' cast_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::MULT, $3);
	}
	| multiplicative_expression '/' cast_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::DIV, $3);
	}
	| multiplicative_expression '%' cast_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::MOD, $3);
	}
	;

additive_expression
	: multiplicative_expression {$$ = $1;}
	| additive_expression '+' multiplicative_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::PLUS, $3);
	}
	| additive_expression '-' multiplicative_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::MINUS, $3);
	}
	;

shift_expression
	: additive_expression {$$ = $1;}
	| shift_expression LEFT_OP additive_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::LSHIFT, $3);
	}
	| shift_expression RIGHT_OP additive_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::RSHIFT, $3);
	}
	;

relational_expression
	: shift_expression {$$ = $1;}
	| relational_expression '<' shift_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::LT, $3);
	}
	| relational_expression '>' shift_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::GT, $3);
	}
	| relational_expression LE_OP shift_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::LTE, $3);
	}
	| relational_expression GE_OP shift_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::GTE, $3);
	}
	;

equality_expression
	: relational_expression {$$ = $1;}
	| equality_expression EQ_OP relational_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::EQUAL, $3);
	}
	| equality_expression NE_OP relational_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::NOT_EQUAL, $3);
	}
	;

and_expression
	: equality_expression {$$ = $1;}
	| and_expression '&' equality_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::AND, $3);
	}
	;

exclusive_or_expression
	: and_expression {$$ = $1;}
	| exclusive_or_expression '^' and_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::XOR, $3);
	}
	;

inclusive_or_expression
	: exclusive_or_expression {$$ = $1;}
	| inclusive_or_expression '|' exclusive_or_expression
	{
       $$ = new yyBinaryOp($1, BinaryOp::OR, $3);
	}
	;

logical_and_expression
	: inclusive_or_expression {$$ = $1;}
	| logical_and_expression AND_OP inclusive_or_expression
    {
       $$ = new yyBinaryOp($1, BinaryOp::LOGICAL_AND, $3);
    }
	;

logical_or_expression
	: logical_and_expression {$$ = $1;}
	| logical_or_expression OR_OP logical_and_expression
	{
	   $$ = new yyBinaryOp($1, BinaryOp::LOGICAL_OR, $3);
	}
	;

conditional_expression
	: logical_or_expression {$$ = $1;}
	| logical_or_expression '?' expression ':' conditional_expression
	;

assignment_expression
	: conditional_expression {$$ = $1;}
	| unary_expression assignment_operator assignment_expression
	;

assignment_operator
	: '='
	| MUL_ASSIGN
	| DIV_ASSIGN
	| MOD_ASSIGN
	| ADD_ASSIGN
	| SUB_ASSIGN
	| LEFT_ASSIGN
	| RIGHT_ASSIGN
	| AND_ASSIGN
	| XOR_ASSIGN
	| OR_ASSIGN
	;

expression
	: assignment_expression {$$ = new yyExpression($1);}
	| expression ',' assignment_expression
	{
	    yyExpression* dollar_1_casted = dynamic_cast<yyExpression*> ($1);
	    assert(dollar_1_casted != nullptr);
	    dollar_1_casted->addAssignmentExpression($3);
	    $$ = $1;
	}
	;

constant_expression
	: conditional_expression	/* with constraints */
	;

declaration
	: declaration_specifiers ';'
	| declaration_specifiers init_declarator_list ';'
	| static_assert_declaration
	;

declaration_specifiers
	: storage_class_specifier declaration_specifiers
	| storage_class_specifier
	| type_specifier declaration_specifiers
	| type_specifier {$$ = new yyDeclSpecifiers($1);}
	| type_qualifier declaration_specifiers
	| type_qualifier
	| function_specifier declaration_specifiers
	| function_specifier
	| alignment_specifier declaration_specifiers
	| alignment_specifier
	;

init_declarator_list
	: init_declarator
	| init_declarator_list ',' init_declarator
	;

init_declarator
	: declarator '=' initializer
	| declarator
	;

storage_class_specifier
	: TYPEDEF	/* identifiers must be flagged as TYPEDEF_NAME */
	| EXTERN
	| STATIC
	| THREAD_LOCAL
	| AUTO
	| REGISTER
	;

type_specifier
	: VOID  {$$ = new yyTypeSpecifier(yySimpleType::TYPE_VOID);}
	| CHAR
	| SHORT
	| INT   {$$ = new yyTypeSpecifier(yySimpleType::TYPE_INT);}
	| LONG
	| FLOAT {$$ = new yyTypeSpecifier(yySimpleType::TYPE_FLOAT);}
	| DOUBLE
	| SIGNED
	| UNSIGNED
	| BOOL
	| COMPLEX
	| IMAGINARY	  	/* non-mandated extension */
	| atomic_type_specifier
	| struct_or_union_specifier
	| enum_specifier
	| TYPEDEF_NAME		/* after it has been defined as such */
	;

struct_or_union_specifier
	: struct_or_union '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER '{' struct_declaration_list '}'
	| struct_or_union IDENTIFIER
	;

struct_or_union
	: STRUCT
	| UNION
	;

struct_declaration_list
	: struct_declaration
	| struct_declaration_list struct_declaration
	;

struct_declaration
	: specifier_qualifier_list ';'	/* for anonymous struct/union */
	| specifier_qualifier_list struct_declarator_list ';'
	| static_assert_declaration
	;

specifier_qualifier_list
	: type_specifier specifier_qualifier_list
	| type_specifier
	| type_qualifier specifier_qualifier_list
	| type_qualifier
	;

struct_declarator_list
	: struct_declarator
	| struct_declarator_list ',' struct_declarator
	;

struct_declarator
	: ':' constant_expression
	| declarator ':' constant_expression
	| declarator
	;

enum_specifier
	: ENUM '{' enumerator_list '}'
	| ENUM '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER '{' enumerator_list '}'
	| ENUM IDENTIFIER '{' enumerator_list ',' '}'
	| ENUM IDENTIFIER
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
	;

enumerator	/* identifiers must be flagged as ENUMERATION_CONSTANT */
	: enumeration_constant '=' constant_expression
	| enumeration_constant
	;

atomic_type_specifier
	: ATOMIC '(' type_name ')'
	;

type_qualifier
	: CONST
	| RESTRICT
	| VOLATILE
	| ATOMIC
	;

function_specifier
	: INLINE
	| NORETURN
	;

alignment_specifier
	: ALIGNAS '(' type_name ')'
	| ALIGNAS '(' constant_expression ')'
	;

declarator
	: pointer direct_declarator
	| direct_declarator
	;

direct_declarator
	: IDENTIFIER      { $$ = new yyDirectDeclarator(new yyIdentifier($1)) ;}
	| '(' declarator ')'
	| direct_declarator '[' ']'
	| direct_declarator '[' '*' ']'
	| direct_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_declarator '[' STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list '*' ']'
	| direct_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_declarator '[' type_qualifier_list ']'
	| direct_declarator '[' assignment_expression ']'
	| direct_declarator '(' parameter_type_list ')' {$$ = new yyDirectDeclarator($1, $3);}
	| direct_declarator '(' ')' {$$ = new yyDirectDeclarator($1, new yyParameterList());}
	| direct_declarator '(' identifier_list ')'
	;

pointer
	: '*' type_qualifier_list pointer
	| '*' type_qualifier_list
	| '*' pointer
	| '*'
	;

type_qualifier_list
	: type_qualifier
	| type_qualifier_list type_qualifier
	;


parameter_type_list
	: parameter_list ',' ELLIPSIS
	| parameter_list {$$ = $1;}
	;

parameter_list
	: parameter_declaration {$$ = new yyParameterList($1);}
	| parameter_list ',' parameter_declaration
	{
	  yyParameterList* dollar_1_casted = dynamic_cast<yyParameterList*> ($1);
      assert(dollar_1_casted != nullptr);
	  dollar_1_casted->addParam($3);
	  $$ = $1;
	}
	;

parameter_declaration
	: declaration_specifiers declarator {$$ = new yyParameterDecl($1, $2);}
	| declaration_specifiers abstract_declarator
	| declaration_specifiers
	;

identifier_list
	: IDENTIFIER
	| identifier_list ',' IDENTIFIER
	;

type_name
	: specifier_qualifier_list abstract_declarator
	| specifier_qualifier_list
	;

abstract_declarator
	: pointer direct_abstract_declarator
	| pointer
	| direct_abstract_declarator
	;

direct_abstract_declarator
	: '(' abstract_declarator ')'
	| '[' ']'
	| '[' '*' ']'
	| '[' STATIC type_qualifier_list assignment_expression ']'
	| '[' STATIC assignment_expression ']'
	| '[' type_qualifier_list STATIC assignment_expression ']'
	| '[' type_qualifier_list assignment_expression ']'
	| '[' type_qualifier_list ']'
	| '[' assignment_expression ']'
	| direct_abstract_declarator '[' ']'
	| direct_abstract_declarator '[' '*' ']'
	| direct_abstract_declarator '[' STATIC type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list STATIC assignment_expression ']'
	| direct_abstract_declarator '[' type_qualifier_list ']'
	| direct_abstract_declarator '[' assignment_expression ']'
	| '(' ')'
	| '(' parameter_type_list ')'
	| direct_abstract_declarator '(' ')'
	| direct_abstract_declarator '(' parameter_type_list ')'
	;

initializer
	: '{' initializer_list '}'
	| '{' initializer_list ',' '}'
	| assignment_expression
	;

initializer_list
	: designation initializer
	| initializer
	| initializer_list ',' designation initializer
	| initializer_list ',' initializer
	;

designation
	: designator_list '='
	;

designator_list
	: designator
	| designator_list designator
	;

designator
	: '[' constant_expression ']'
	| '.' IDENTIFIER
	;

static_assert_declaration
	: STATIC_ASSERT '(' constant_expression ',' STRING_LITERAL ')' ';'
	;

statement
	: labeled_statement
	| compound_statement
	| expression_statement {$$ = $1;}
	| selection_statement
	| iteration_statement
	| jump_statement
	;

labeled_statement
	: IDENTIFIER ':' statement
	| CASE constant_expression ':' statement
	| DEFAULT ':' statement
	;

compound_statement
	: '{' '}'                   {$$ = new yyCompoundStatement();}
	| '{'  block_item_list '}'
    {
        yyCompoundStatement* cmpndStatement = new yyCompoundStatement();

        for (auto item: $2->nodes) {
            cmpndStatement->addItem(item);
        }

        $$ = cmpndStatement;
    }
	;

block_item_list
	: block_item {$$ = new yyAST(); $$->addNode($1);}
	| block_item_list block_item {$1->addNode($2); $$ = $1;}
	;

block_item
	: declaration {$$ = $1;}
	| statement   {$$ = $1;}
	;

expression_statement
	: ';'  {$$ = new yyExpression();}
	| expression ';'  {$$ = $1;}
	;

selection_statement
	: IF '(' expression ')' statement ELSE statement
	| IF '(' expression ')' statement
	| SWITCH '(' expression ')' statement
	;

iteration_statement
	: WHILE '(' expression ')' statement
	| DO statement WHILE '(' expression ')' ';'
	| FOR '(' expression_statement expression_statement ')' statement
	| FOR '(' expression_statement expression_statement expression ')' statement
	| FOR '(' declaration expression_statement ')' statement
	| FOR '(' declaration expression_statement expression ')' statement
	;

jump_statement
	: GOTO IDENTIFIER ';'
	| CONTINUE ';'
	| BREAK ';'
	| RETURN ';'
	| RETURN expression ';'
	;

translation_unit
	: external_declaration
	{
	    topLevelTU->addDecl($1);
        $$ = topLevelTU;
    }
	| translation_unit external_declaration
	{
	    topLevelTU->addDecl($2);
        $$ = topLevelTU;
    }
	;

external_declaration
	: function_definition
	{
	    $$ = $1;
	}
	| declaration
	{
	    std::cout << "TODO: external_declaration prod 2\n";
	    $$ = new yyDeclaration();
	    // $$ = $1;
	}
	;

function_definition
	: declaration_specifiers declarator declaration_list compound_statement

	| declaration_specifiers declarator compound_statement
	{
        $$ = new yyFunctionDefinition($1, $2, $3);
	}
	;

declaration_list
	: declaration
	| declaration_list declaration
	;

%%
#include <stdio.h>

void yyerror(const char *s)
{
	fflush(stdout);
	fprintf(stderr, "*** %s\n", s);
}

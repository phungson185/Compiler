/* 
 * @copyright (c) 2008, Hedspi, Hanoi University of Technology
 * @author Huu-Duc Nguyen
 * @version 1.0
 */
#include <stdio.h>
#include <stdlib.h>

#include "reader.h"
#include "scanner.h"
#include "parser.h"
#include "semantics.h"
#include "error.h"
#include "debug.h"

Token *currentToken;
Token *lookAhead;

extern Type *intType;
extern Type *charType;
extern SymTab *symtab;

void scan(void)
{
  Token *tmp = currentToken;
  currentToken = lookAhead;
  lookAhead = getValidToken();
  free(tmp);
}

void eat(TokenType tokenType)
{
  if (lookAhead->tokenType == tokenType)
  {
    scan();
  }
  else
    missingToken(tokenType, lookAhead->lineNo, lookAhead->colNo);
}

void compileProgram(void)
{
  Object *program;

  eat(KW_PROGRAM);
  eat(TK_IDENT);

  program = createProgramObject(currentToken->string);
  enterBlock(program->progAttrs->scope);

  eat(SB_SEMICOLON);

  compileBlock();
  eat(SB_PERIOD);

  exitBlock();
}

void compileBlock(void)
{
  Object *constObj;
  ConstantValue *constValue;

  if (lookAhead->tokenType == KW_CONST)
  {
    eat(KW_CONST);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      constObj = createConstantObject(currentToken->string);

      eat(SB_EQ);
      constValue = compileConstant();

      constObj->constAttrs->value = constValue;
      declareObject(constObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock2();
  }
  else
    compileBlock2();
}

void compileBlock2(void)
{
  Object *typeObj;
  Type *actualType;

  if (lookAhead->tokenType == KW_TYPE)
  {
    eat(KW_TYPE);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      typeObj = createTypeObject(currentToken->string);

      eat(SB_EQ);
      actualType = compileType();

      typeObj->typeAttrs->actualType = actualType;
      declareObject(typeObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock3();
  }
  else
    compileBlock3();
}

void compileBlock3(void)
{
  Object *varObj;
  Type *varType;

  if (lookAhead->tokenType == KW_VAR)
  {
    eat(KW_VAR);

    do
    {
      eat(TK_IDENT);

      checkFreshIdent(currentToken->string);
      varObj = createVariableObject(currentToken->string);

      eat(SB_COLON);
      varType = compileType();

      varObj->varAttrs->type = varType;
      declareObject(varObj);

      eat(SB_SEMICOLON);
    } while (lookAhead->tokenType == TK_IDENT);

    compileBlock4();
  }
  else
    compileBlock4();
}

void compileBlock4(void)
{
  compileSubDecls();
  compileBlock5();
}

void compileBlock5(void)
{
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileSubDecls(void)
{
  while ((lookAhead->tokenType == KW_FUNCTION) || (lookAhead->tokenType == KW_PROCEDURE))
  {
    if (lookAhead->tokenType == KW_FUNCTION)
      compileFuncDecl();
    else
      compileProcDecl();
  }
}

void compileFuncDecl(void)
{
  Object *funcObj;
  Type *returnType;

  eat(KW_FUNCTION);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  funcObj = createFunctionObject(currentToken->string);
  declareObject(funcObj);

  enterBlock(funcObj->funcAttrs->scope);

  compileParams();
  eat(SB_COLON);
  returnType = compileBasicType();
  funcObj->funcAttrs->returnType = returnType;

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();
}

void compileProcDecl(void)
{
  Object *procObj;

  eat(KW_PROCEDURE);
  eat(TK_IDENT);

  checkFreshIdent(currentToken->string);
  procObj = createProcedureObject(currentToken->string);
  declareObject(procObj);

  enterBlock(procObj->procAttrs->scope);

  compileParams();

  eat(SB_SEMICOLON);
  compileBlock();
  eat(SB_SEMICOLON);

  exitBlock();
}

ConstantValue *compileUnsignedConstant(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_INTEGER:
    eat(TK_INTEGER);
    constValue = makeIntConstant(currentToken->intValue);
    break;
  case TK_IDENT:
    eat(TK_IDENT);

    obj = checkDeclaredConstant(currentToken->string);
    constValue = duplicateConstantValue(obj->constAttrs->value);

    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    constValue = makeDoubleConstant(currentToken->doubleValue);
    break;

  case TK_STRING:
    eat(TK_STRING);
    constValue = makeStringConstant(currentToken->string);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

ConstantValue *compileConstant(void)
{
  ConstantValue *constValue;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    constValue = compileConstant2();
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    constValue = compileConstant2();
    constValue->intValue = -constValue->intValue;
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    constValue = makeCharConstant(currentToken->string[0]);
    break;
  default:
    constValue = compileConstant2();
    break;
  }
  return constValue;
}

ConstantValue *compileConstant2(void)
{
  ConstantValue *constValue;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case TK_INTEGER:
    eat(TK_INTEGER);
    constValue = makeIntConstant(currentToken->intValue);
    break;
  case TK_STRING:
    eat(TK_STRING);
    constValue = makeStringConstant(currentToken->string);
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    constValue = makeDoubleConstant(currentToken->doubleValue);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredConstant(currentToken->string);
    if (obj->constAttrs->value->type != TP_ARRAY)
      constValue = duplicateConstantValue(obj->constAttrs->value);
    break;
  default:
    error(ERR_INVALID_CONSTANT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return constValue;
}

Type *compileType(void)
{
  Type *type;
  Type *elementType;
  int arraySize;
  Object *obj;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_DOUBLE:
    eat(KW_DOUBLE);
    type = makeDoubleType();
    break;
  case KW_STRING:
    eat(KW_STRING);
    type = makeStringType();
    break;
  case KW_ARRAY:
    eat(KW_ARRAY);
    eat(SB_LSEL);
    eat(TK_INTEGER);
    arraySize = currentToken->intValue;

    eat(SB_RSEL);
    eat(KW_OF);
    elementType = compileType();
    type = makeArrayType(arraySize, elementType);
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    obj = checkDeclaredType(currentToken->string);
    type = duplicateType(obj->typeAttrs->actualType);
    break;
  default:
    error(ERR_INVALID_TYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

Type *compileBasicType(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case KW_INTEGER:
    eat(KW_INTEGER);
    type = makeIntType();
    break;
  case KW_DOUBLE:
    eat(KW_DOUBLE);
    type = makeDoubleType();
    break;
  case KW_CHAR:
    eat(KW_CHAR);
    type = makeCharType();
    break;
  case KW_STRING:
    eat(KW_STRING);
    type = makeStringType();
    break;
  default:
    error(ERR_INVALID_BASICTYPE, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
  return type;
}

void compileParams(void)
{
  if (lookAhead->tokenType == SB_LPAR)
  {
    eat(SB_LPAR);
    compileParam();
    while (lookAhead->tokenType == SB_SEMICOLON)
    {
      eat(SB_SEMICOLON);
      compileParam();
    }
    eat(SB_RPAR);
  }
}

void compileParam(void)
{
  Object *param;
  Type *type;
  enum ParamKind paramKind;

  switch (lookAhead->tokenType)
  {
  case TK_IDENT:
    paramKind = PARAM_VALUE;
    break;
  case KW_VAR:
    eat(KW_VAR);
    paramKind = PARAM_REFERENCE;
    break;
  default:
    error(ERR_INVALID_PARAMETER, lookAhead->lineNo, lookAhead->colNo);
    break;
  }

  eat(TK_IDENT);
  checkFreshIdent(currentToken->string);
  param = createParameterObject(currentToken->string, paramKind, symtab->currentScope->owner);
  eat(SB_COLON);
  type = compileBasicType();
  param->paramAttrs->type = type;
  declareObject(param);
}

void compileStatements(void)
{
  compileStatement();
  while (lookAhead->tokenType == SB_SEMICOLON)
  {
    eat(SB_SEMICOLON);
    compileStatement();
  }
}

void compileStatement(void)
{
  switch (lookAhead->tokenType)
  {
  case TK_IDENT:
    compileAssignSt();
    break;
  case KW_CALL:
    compileCallSt();
    break;
  case KW_BEGIN:
    compileGroupSt();
    break;
  case KW_IF:
    compileIfSt();
    break;
  case KW_WHILE:
    compileWhileSt();
    break;
  case KW_FOR:
    compileForSt();
    break;
    // EmptySt needs to check FOLLOW tokens
  case KW_DO:
    compileDoWhileSt();
    break;
  case KW_SWITCH:
    compileSwitchSt();
    break;
  case KW_CASE:
    compileCaseSt();
    break;
  case KW_DEFAULT:
    compileDefaultSt();
    break;
  case KW_BREAK:
    eat(KW_BREAK);
    break;
  case KW_REPEAT:
    compileRepeatUntilSt();
    break;
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
    break;
    // Error occurs
  default:
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
    break;
  }
}

Type *compileLValue(void)
{
  // TODO: parse a lvalue (a variable, an array element, a parameter, the current function identifier)
  Object *var;
  Type *varType;
  switch (lookAhead->tokenType)
  {
  case TK_CHAR:
  case TK_DOUBLE:
  case TK_STRING:
  case TK_INTEGER:
    error(ERR_INVALID_VARIABLE, currentToken->lineNo, currentToken->colNo);
    break;
  default:
    break;
  }
  eat(TK_IDENT);
  // check if the identifier is a function identifier, or a variable identifier, or a parameter
  var = checkDeclaredLValueIdent(currentToken->string);
  if (var->kind == OBJ_VARIABLE)
  {
    if (var->varAttrs->type->typeClass == TP_ARRAY)
      varType = compileIndexes(var->varAttrs->type);
    else
      varType = duplicateType(var->varAttrs->type);
  }
  else if (var->kind == OBJ_PARAMETER)
    varType = duplicateType(var->paramAttrs->type);
  else
    varType = duplicateType(var->funcAttrs->returnType);
  return varType;
}

void compileAssignSt(void)
{
  // TODO: parse the assignment and check type consistency
  int leftCount = 0;
  int rightCount = 0;
  Type **leftType = (Type **)malloc(100 * sizeof(Type *));
  Type **rightType = (Type **)malloc(100 * sizeof(Type *));
  while (1)
  {
    leftType[leftCount] = compileLValue();
    leftCount += 1;
    if (lookAhead->tokenType != SB_COMMA)
      break;
    eat(SB_COMMA);
  }
  eat(SB_ASSIGN);
  while (1)
  {
    rightType[rightCount] = compileExpression();
    rightCount += 1;
    if (lookAhead->tokenType != SB_COMMA)
      break;
    eat(SB_COMMA);
  }
  checkVariableCount(leftCount, rightCount);
  for (int i = 0; i < leftCount; i++)
    checkTypeEquality(*(leftType + i), *(rightType + i));
}

void compileCallSt(void)
{
  Object *proc;

  eat(KW_CALL);
  eat(TK_IDENT);

  proc = checkDeclaredProcedure(currentToken->string);

  compileArguments(proc->procAttrs->paramList);
}

void compileGroupSt(void)
{
  eat(KW_BEGIN);
  compileStatements();
  eat(KW_END);
}

void compileIfSt(void)
{
  eat(KW_IF);
  compileCondition();
  eat(KW_THEN);
  compileStatement();
  if (lookAhead->tokenType == KW_ELSE)
    compileElseSt();
}

void compileElseSt(void)
{
  eat(KW_ELSE);
  compileStatement();
}

void compileWhileSt(void)
{
  eat(KW_WHILE);
  compileCondition();
  eat(KW_DO);
  compileStatement();
}

void compileDoWhileSt(void)
{
  eat(KW_DO);
  compileStatement();
  eat(KW_WHILE);
  compileCondition();
}

void compileForSt(void)
{
  // TODO: Check type consistency of FOR's variable
  Object *varObj;
  Type *expType;

  eat(KW_FOR);
  eat(TK_IDENT);

  // check if the identifier is a variable
  varObj = checkDeclaredVariable(currentToken->string);

  eat(SB_ASSIGN);
  expType = compileExpression();
  checkTypeEquality(varObj->varAttrs->type, expType);

  eat(KW_TO);
  expType = compileExpression();
  checkTypeEquality(varObj->varAttrs->type, expType);

  eat(KW_DO);
  compileStatement();
}

void compileSwitchSt(void)
{
  eat(KW_SWITCH);
  compileExpression();
  compileGroupSt();
}

void compileCaseSt(void)
{
  eat(KW_CASE);
  compileConstant();
  if (lookAhead->tokenType == SB_COLON)
    eat(SB_COLON);
  else
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
  compileStatements();
}

void compileDefaultSt(void)
{
  eat(KW_DEFAULT);
  if (lookAhead->tokenType == SB_COLON)
    eat(SB_COLON);
  else
    error(ERR_INVALID_STATEMENT, lookAhead->lineNo, lookAhead->colNo);
  compileStatement();
}

void compileRepeatUntilSt(void){
  eat(KW_REPEAT);
  compileStatement();
  eat(KW_UNTIL);
  compileCondition();
}

void compileArgument(Object *param)
{
  // TODO: parse an argument, and check type consistency
  //       If the corresponding parameter is a reference, the argument must be a lvalue
  Type *expType;

  if (param != NULL && param->paramAttrs->kind == PARAM_VALUE)
  {
    expType = compileExpression();
    checkTypeEquality(expType, param->paramAttrs->type);
  }
  else if (param != NULL && param->paramAttrs->kind == PARAM_REFERENCE)
  {
    expType = compileLValue();
    checkTypeEquality(expType, param->paramAttrs->type);
  }
  else if (param == NULL && currentToken->tokenType != SB_RPAR)
    error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);
}

void compileArguments(ObjectNode *paramList)
{
  //TODO: parse a list of arguments, check the consistency of the arguments and the given parameters
  ObjectNode *node = paramList;
  Object *param;
  if (node == NULL)
    param = NULL;
  else
    param = node->object;

  switch (lookAhead->tokenType)
  {
  case SB_LPAR:
    eat(SB_LPAR);
    compileArgument(param);

    while (lookAhead->tokenType == SB_COMMA)
    {
      eat(SB_COMMA);
      node = node->next;
      if (node == NULL)
        param = NULL;
      else
        param = node->object;
      compileArgument(param);
    }

    if (node->next != NULL)
      error(ERR_PARAMETERS_ARGUMENTS_INCONSISTENCY, currentToken->lineNo, currentToken->colNo);

    eat(SB_RPAR);
    break;
    // Check FOLLOW set
  case SB_TIMES:
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_ARGUMENTS, lookAhead->lineNo, lookAhead->colNo);
  }
}

void compileCondition(void)
{
  // TODO: check the type consistency of LHS and RSH, check the basic type
  Type *leftType = compileExpression();

  switch (lookAhead->tokenType)
  {
  case SB_EQ:
    eat(SB_EQ);
    break;
  case SB_NEQ:
    eat(SB_NEQ);
    break;
  case SB_LE:
    eat(SB_LE);
    break;
  case SB_LT:
    eat(SB_LT);
    break;
  case SB_GE:
    eat(SB_GE);
    break;
  case SB_GT:
    eat(SB_GT);
    break;
  default:
    error(ERR_INVALID_COMPARATOR, lookAhead->lineNo, lookAhead->colNo);
  }

  Type *rightType = compileExpression();
  checkTypeEquality(leftType, rightType);
}

Type *compileExpression(void)
{
  Type *type;
  Type *type1, *type2;
  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    type = compileExpression2();
    if (type->typeClass == TP_STRING)
      error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type = compileExpression2();
    if (type->typeClass == TP_STRING)
      error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    break;
  case KW_IF:
    eat(KW_IF);
    compileCondition();
    eat(KW_RETURN);

    type1 = compileExpression();

    eat(KW_ELSE);
    eat(KW_RETURN);

    type2 = compileExpression();
    checkTypeEquality(type2, type1);
    return type1;
  case KW_SUM:
    type= compileSum();
    break;
  default:
    type = compileExpression2();
  }
  return type;
}

Type *compileExpression2(void)
{
  Type *type1;
  Type *type2;

  type1 = compileTerm();
  type2 = compileExpression3();
  if (type2 == NULL)
    return type1;
  else
  {
    checkTypeEquality(type1, type2);
    return type1;
  }
}

Type *compileExpression3(void)
{
  Type *type1;
  Type *type2;

  switch (lookAhead->tokenType)
  {
  case SB_PLUS:
    eat(SB_PLUS);
    type1 = compileTerm();
    type2 = compileExpression3();
    if (type2 != NULL)
      checkTypeEquality(type1, type2);
    return type1;
    break;
  case SB_MINUS:
    eat(SB_MINUS);
    type1 = compileTerm();
    if (type1->typeClass == TP_STRING)
      error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    type2 = compileExpression3();
    if (type2 != NULL)
      checkTypeEquality(type1, type2);
    return type1;
    break;
    // check the FOLLOW set
  case KW_BEGIN:
  case KW_RETURN:
  case KW_UNTIL:
  case KW_TO:
  case KW_DO:
  case KW_WHILE:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    return NULL;
    break;
  default:
    error(ERR_INVALID_EXPRESSION, lookAhead->lineNo, lookAhead->colNo);
    return NULL;
  }
}

Type *compileTerm(void)
{
  // TODO: check type of Term2
  Type *type;

  type = compileFactor();
  compileTerm2();

  return type;
}

void compileTerm2(void)
{
  // TODO: check type of term2
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_TIMES:
    eat(SB_TIMES);
    type = compileFactor();
    if (type->typeClass == TP_DOUBLE)
      checkDoubleType(type);
    else
      checkIntType(type);
    compileTerm2();
    break;
  case SB_SLASH:
    eat(SB_SLASH);
    type = compileFactor();
    if (type->typeClass == TP_DOUBLE)
      checkDoubleType(type);
    else
      checkIntType(type);
    compileTerm2();
    break;
    // check the FOLLOW set
  case KW_RETURN:
  case KW_BEGIN:
  case KW_UNTIL:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case KW_WHILE:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
  }
}

Type* compileSum(void){
    eat(KW_SUM);
    Type* type = compileExpression();
    if(type == NULL)
      return makeIntType();
    checkIntType(type);
    while (lookAhead->tokenType == SB_COMMA){
        eat(SB_COMMA);
        type= compileExpression();
        checkIntType(type);
    }
    return type;
}

Type *compileFactor(void)
{
  // TODO: parse a factor and return the factor's type

  Object *obj;
  Type *type;
  Type *type1;
  switch (lookAhead->tokenType)
  {
  case TK_INTEGER:
    eat(TK_INTEGER);

    if (lookAhead->tokenType == SB_POW)
    {
      type = compilePow();
      return type;
    }
    else
    {
      type = makeIntType();
      return type;
    }
    break;
  case TK_CHAR:
    eat(TK_CHAR);
    type = makeCharType();
    return type;
    break;
  case TK_DOUBLE:
    eat(TK_DOUBLE);
    type = makeDoubleType();
    return type;
    break;
  case TK_STRING:
    eat(TK_STRING);
    type = makeStringType();
    return type;
    break;
  case TK_IDENT:
    eat(TK_IDENT);
    // check if the identifier is declared
    obj = checkDeclaredIdent(currentToken->string);

    switch (obj->kind)
    {
    case OBJ_CONSTANT:
      switch (obj->constAttrs->value->type)
      {
      case TP_INT:
        type = makeIntType();
        break;
      case TP_CHAR:
        type = makeCharType();
        break;
      case TP_DOUBLE:
        type = makeDoubleType();
        break;
      case TP_STRING:
        type = makeStringType();
        break;
      case TP_ARRAY:
        error(ERR_INVALID_CONSTANT, currentToken->lineNo, currentToken->colNo);
        break;
      }
      break;
    case OBJ_VARIABLE:
      if (obj->varAttrs->type->typeClass == TP_ARRAY)
        type = compileIndexes(obj->varAttrs->type);
      else
      {
        if (lookAhead->tokenType == SB_POW)
        {
          type1 = duplicateType(obj->varAttrs->type);
          type = compilePow();
          checkTypeEquality(type1, type);
          break;
        }
        else
        {
          type = duplicateType(obj->varAttrs->type);
          break;
        }
      }
    case OBJ_PARAMETER:
      type = duplicateType(obj->paramAttrs->type);

      break;
    case OBJ_FUNCTION:
      compileArguments(obj->funcAttrs->paramList);
      type = duplicateType(obj->funcAttrs->returnType);
      break;
    default:
      return NULL;
    }
    break;
  default:
      return NULL;
  }

  return type;
}

Type *compilePow(void)
{
  Type *type;

  switch (lookAhead->tokenType)
  {
  case SB_POW:
    eat(SB_POW);
    type = compileFactor();
    checkIntType(type);
    return type;
    break;
  case SB_SLASH:
  case SB_PLUS:
  case SB_MINUS:
  case KW_TO:
  case KW_DO:
  case KW_WHILE:
  case SB_RPAR:
  case SB_COMMA:
  case SB_EQ:
  case SB_NEQ:
  case SB_LE:
  case SB_LT:
  case SB_GE:
  case SB_GT:
  case SB_RSEL:
  case SB_SEMICOLON:
  case KW_END:
  case KW_ELSE:
  case KW_THEN:
    return NULL;
    break;
  default:
    error(ERR_INVALID_TERM, lookAhead->lineNo, lookAhead->colNo);
    return NULL;
  }
}

Type *compileIndexes(Type *arrayType)
{
  // TODO: parse a sequence of indexes, check the consistency to the arrayType, and return the element type
  Type *expressionType;
  while (lookAhead->tokenType == SB_LSEL)
  {
    eat(SB_LSEL);
    expressionType = compileExpression();
    checkIntType(expressionType);
    // if(arrayType->typeClass == TP_ARRAY)
    arrayType = arrayType->elementType;
    // else error(ERR_INDEXES_EXCEED_DIMENSION, currentToken->lineNo, currentToken->colNo);
    eat(SB_RSEL);
  }
  return arrayType;
}

int compile(char *fileName)
{
  if (openInputStream(fileName) == IO_ERROR)
    return IO_ERROR;

  currentToken = NULL;
  lookAhead = getValidToken();

  initSymTab();

  compileProgram();

  printObject(symtab->program, 0);

  cleanSymTab();

  free(currentToken);
  free(lookAhead);
  closeInputStream();
  return IO_SUCCESS;
}
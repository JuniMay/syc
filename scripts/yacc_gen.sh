# generate yacc parser
cd src
if test -d "frontend/generated"; then
  rm -rf frontend/generated
fi
mkdir frontend/generated
flex --header-file=frontend/generated/lexer.h -o frontend/generated/lexer.cpp frontend/lexer.l
bison -d -v -o frontend/generated/parser.cpp --defines=frontend/generated/parser.h frontend/parser.y -Wcounterexamples
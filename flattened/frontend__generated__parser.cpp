// A Bison parser, made by GNU Bison 3.8.2.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2021 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
// especially those whose name start with YY_ or yy_.  They are
// private implementation details that can be changed or removed.





#include "parser.h"


// Unqualified %code blocks.
#line 36 "frontend/parser.y"

  syc::frontend::Parser::symbol_type yylex(
    yyscan_t yyscanner, 
    syc::frontend::location& loc, 
    syc::frontend::Driver& driver
  );

#line 54 "frontend/generated/parser.cpp"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif


// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yy_stack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YY_USE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 20 "frontend/parser.y"
namespace  syc { namespace frontend  {
#line 147 "frontend/generated/parser.cpp"

  /// Build a parser object.
   Parser :: Parser  (yyscan_t yyscanner_yyarg, location& loc_yyarg, Driver& driver_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      yyscanner (yyscanner_yyarg),
      loc (loc_yyarg),
      driver (driver_yyarg)
  {}

   Parser ::~ Parser  ()
  {}

   Parser ::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------.
  | symbol.  |
  `---------*/



  // by_state.
   Parser ::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

   Parser ::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
   Parser ::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
   Parser ::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

   Parser ::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

   Parser ::symbol_kind_type
   Parser ::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

   Parser ::stack_symbol_type::stack_symbol_type ()
  {}

   Parser ::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.YY_MOVE_OR_COPY< ComptimeValue > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Type: // Type
        value.YY_MOVE_OR_COPY< TypePtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_InitVal: // InitVal
      case symbol_kind::S_Expr: // Expr
      case symbol_kind::S_Cond: // Cond
      case symbol_kind::S_PrimaryExpr: // PrimaryExpr
      case symbol_kind::S_LVal: // LVal
      case symbol_kind::S_UnaryExpr: // UnaryExpr
      case symbol_kind::S_MulExpr: // MulExpr
      case symbol_kind::S_AddExpr: // AddExpr
      case symbol_kind::S_RelExpr: // RelExpr
      case symbol_kind::S_EqExpr: // EqExpr
      case symbol_kind::S_LAndExpr: // LAndExpr
      case symbol_kind::S_LOrExpr: // LOrExpr
        value.YY_MOVE_OR_COPY< ast::ExprPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Stmt: // Stmt
      case symbol_kind::S_ExprStmt: // ExprStmt
      case symbol_kind::S_DeclStmt: // DeclStmt
      case symbol_kind::S_IfStmt: // IfStmt
      case symbol_kind::S_WhileStmt: // WhileStmt
      case symbol_kind::S_ReturnStmt: // ReturnStmt
      case symbol_kind::S_ContinueStmt: // ContinueStmt
      case symbol_kind::S_BreakStmt: // BreakStmt
      case symbol_kind::S_BlockStmt: // BlockStmt
      case symbol_kind::S_BlankStmt: // BlankStmt
      case symbol_kind::S_AssignStmt: // AssignStmt
        value.YY_MOVE_OR_COPY< ast::StmtPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Def: // Def
        value.YY_MOVE_OR_COPY< std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.YY_MOVE_OR_COPY< std::tuple<TypePtr, std::string> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.YY_MOVE_OR_COPY< std::vector<ast::ExprPtr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DefList: // DefList
        value.YY_MOVE_OR_COPY< std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.YY_MOVE_OR_COPY< std::vector<std::tuple<TypePtr, std::string>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

   Parser ::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.move< ComptimeValue > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Type: // Type
        value.move< TypePtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_InitVal: // InitVal
      case symbol_kind::S_Expr: // Expr
      case symbol_kind::S_Cond: // Cond
      case symbol_kind::S_PrimaryExpr: // PrimaryExpr
      case symbol_kind::S_LVal: // LVal
      case symbol_kind::S_UnaryExpr: // UnaryExpr
      case symbol_kind::S_MulExpr: // MulExpr
      case symbol_kind::S_AddExpr: // AddExpr
      case symbol_kind::S_RelExpr: // RelExpr
      case symbol_kind::S_EqExpr: // EqExpr
      case symbol_kind::S_LAndExpr: // LAndExpr
      case symbol_kind::S_LOrExpr: // LOrExpr
        value.move< ast::ExprPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Stmt: // Stmt
      case symbol_kind::S_ExprStmt: // ExprStmt
      case symbol_kind::S_DeclStmt: // DeclStmt
      case symbol_kind::S_IfStmt: // IfStmt
      case symbol_kind::S_WhileStmt: // WhileStmt
      case symbol_kind::S_ReturnStmt: // ReturnStmt
      case symbol_kind::S_ContinueStmt: // ContinueStmt
      case symbol_kind::S_BreakStmt: // BreakStmt
      case symbol_kind::S_BlockStmt: // BlockStmt
      case symbol_kind::S_BlankStmt: // BlankStmt
      case symbol_kind::S_AssignStmt: // AssignStmt
        value.move< ast::StmtPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.move< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Def: // Def
        value.move< std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.move< std::tuple<TypePtr, std::string> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.move< std::vector<ast::ExprPtr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DefList: // DefList
        value.move< std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.move< std::vector<std::tuple<TypePtr, std::string>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
   Parser ::stack_symbol_type&
   Parser ::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.copy< ComptimeValue > (that.value);
        break;

      case symbol_kind::S_Type: // Type
        value.copy< TypePtr > (that.value);
        break;

      case symbol_kind::S_InitVal: // InitVal
      case symbol_kind::S_Expr: // Expr
      case symbol_kind::S_Cond: // Cond
      case symbol_kind::S_PrimaryExpr: // PrimaryExpr
      case symbol_kind::S_LVal: // LVal
      case symbol_kind::S_UnaryExpr: // UnaryExpr
      case symbol_kind::S_MulExpr: // MulExpr
      case symbol_kind::S_AddExpr: // AddExpr
      case symbol_kind::S_RelExpr: // RelExpr
      case symbol_kind::S_EqExpr: // EqExpr
      case symbol_kind::S_LAndExpr: // LAndExpr
      case symbol_kind::S_LOrExpr: // LOrExpr
        value.copy< ast::ExprPtr > (that.value);
        break;

      case symbol_kind::S_Stmt: // Stmt
      case symbol_kind::S_ExprStmt: // ExprStmt
      case symbol_kind::S_DeclStmt: // DeclStmt
      case symbol_kind::S_IfStmt: // IfStmt
      case symbol_kind::S_WhileStmt: // WhileStmt
      case symbol_kind::S_ReturnStmt: // ReturnStmt
      case symbol_kind::S_ContinueStmt: // ContinueStmt
      case symbol_kind::S_BreakStmt: // BreakStmt
      case symbol_kind::S_BlockStmt: // BlockStmt
      case symbol_kind::S_BlankStmt: // BlankStmt
      case symbol_kind::S_AssignStmt: // AssignStmt
        value.copy< ast::StmtPtr > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.copy< std::string > (that.value);
        break;

      case symbol_kind::S_Def: // Def
        value.copy< std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > (that.value);
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.copy< std::tuple<TypePtr, std::string> > (that.value);
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.copy< std::vector<ast::ExprPtr> > (that.value);
        break;

      case symbol_kind::S_DefList: // DefList
        value.copy< std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > (that.value);
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.copy< std::vector<std::tuple<TypePtr, std::string>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

   Parser ::stack_symbol_type&
   Parser ::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.move< ComptimeValue > (that.value);
        break;

      case symbol_kind::S_Type: // Type
        value.move< TypePtr > (that.value);
        break;

      case symbol_kind::S_InitVal: // InitVal
      case symbol_kind::S_Expr: // Expr
      case symbol_kind::S_Cond: // Cond
      case symbol_kind::S_PrimaryExpr: // PrimaryExpr
      case symbol_kind::S_LVal: // LVal
      case symbol_kind::S_UnaryExpr: // UnaryExpr
      case symbol_kind::S_MulExpr: // MulExpr
      case symbol_kind::S_AddExpr: // AddExpr
      case symbol_kind::S_RelExpr: // RelExpr
      case symbol_kind::S_EqExpr: // EqExpr
      case symbol_kind::S_LAndExpr: // LAndExpr
      case symbol_kind::S_LOrExpr: // LOrExpr
        value.move< ast::ExprPtr > (that.value);
        break;

      case symbol_kind::S_Stmt: // Stmt
      case symbol_kind::S_ExprStmt: // ExprStmt
      case symbol_kind::S_DeclStmt: // DeclStmt
      case symbol_kind::S_IfStmt: // IfStmt
      case symbol_kind::S_WhileStmt: // WhileStmt
      case symbol_kind::S_ReturnStmt: // ReturnStmt
      case symbol_kind::S_ContinueStmt: // ContinueStmt
      case symbol_kind::S_BreakStmt: // BreakStmt
      case symbol_kind::S_BlockStmt: // BlockStmt
      case symbol_kind::S_BlankStmt: // BlankStmt
      case symbol_kind::S_AssignStmt: // AssignStmt
        value.move< ast::StmtPtr > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.move< std::string > (that.value);
        break;

      case symbol_kind::S_Def: // Def
        value.move< std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > (that.value);
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.move< std::tuple<TypePtr, std::string> > (that.value);
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.move< std::vector<ast::ExprPtr> > (that.value);
        break;

      case symbol_kind::S_DefList: // DefList
        value.move< std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > (that.value);
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.move< std::vector<std::tuple<TypePtr, std::string>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
   Parser ::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
   Parser ::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YY_USE (yyoutput);
    if (yysym.empty ())
      yyo << "empty symbol";
    else
      {
        symbol_kind_type yykind = yysym.kind ();
        yyo << (yykind < YYNTOKENS ? "token" : "nterm")
            << ' ' << yysym.name () << " ("
            << yysym.location << ": ";
        YY_USE (yykind);
        yyo << ')';
      }
  }
#endif

  void
   Parser ::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
   Parser ::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
   Parser ::yypop_ (int n) YY_NOEXCEPT
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
   Parser ::debug_stream () const
  {
    return *yycdebug_;
  }

  void
   Parser ::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


   Parser ::debug_level_type
   Parser ::debug_level () const
  {
    return yydebug_;
  }

  void
   Parser ::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

   Parser ::state_type
   Parser ::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
   Parser ::yy_pact_value_is_default_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yypact_ninf_;
  }

  bool
   Parser ::yy_table_value_is_error_ (int yyvalue) YY_NOEXCEPT
  {
    return yyvalue == yytable_ninf_;
  }

  int
   Parser ::operator() ()
  {
    return parse ();
  }

  int
   Parser ::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';
    YY_STACK_PRINT ();

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token\n";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            symbol_type yylookahead (yylex (yyscanner, loc, driver));
            yyla.move (yylookahead);
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    if (yyla.kind () == symbol_kind::S_YYerror)
    {
      // The scanner already issued an error message, process directly
      // to error recovery.  But do not keep the error token as
      // lookahead, it is too special and may lead us to an endless
      // loop in error recovery. */
      yyla.kind_ = symbol_kind::S_YYUNDEF;
      goto yyerrlab1;
    }

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.kind ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.kind ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* Variants are always initialized to an empty instance of the
         correct type. The default '$$ = $1' action is NOT applied
         when using variants.  */
      switch (yyr1_[yyn])
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        yylhs.value.emplace< ComptimeValue > ();
        break;

      case symbol_kind::S_Type: // Type
        yylhs.value.emplace< TypePtr > ();
        break;

      case symbol_kind::S_InitVal: // InitVal
      case symbol_kind::S_Expr: // Expr
      case symbol_kind::S_Cond: // Cond
      case symbol_kind::S_PrimaryExpr: // PrimaryExpr
      case symbol_kind::S_LVal: // LVal
      case symbol_kind::S_UnaryExpr: // UnaryExpr
      case symbol_kind::S_MulExpr: // MulExpr
      case symbol_kind::S_AddExpr: // AddExpr
      case symbol_kind::S_RelExpr: // RelExpr
      case symbol_kind::S_EqExpr: // EqExpr
      case symbol_kind::S_LAndExpr: // LAndExpr
      case symbol_kind::S_LOrExpr: // LOrExpr
        yylhs.value.emplace< ast::ExprPtr > ();
        break;

      case symbol_kind::S_Stmt: // Stmt
      case symbol_kind::S_ExprStmt: // ExprStmt
      case symbol_kind::S_DeclStmt: // DeclStmt
      case symbol_kind::S_IfStmt: // IfStmt
      case symbol_kind::S_WhileStmt: // WhileStmt
      case symbol_kind::S_ReturnStmt: // ReturnStmt
      case symbol_kind::S_ContinueStmt: // ContinueStmt
      case symbol_kind::S_BreakStmt: // BreakStmt
      case symbol_kind::S_BlockStmt: // BlockStmt
      case symbol_kind::S_BlankStmt: // BlankStmt
      case symbol_kind::S_AssignStmt: // AssignStmt
        yylhs.value.emplace< ast::StmtPtr > ();
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        yylhs.value.emplace< std::string > ();
        break;

      case symbol_kind::S_Def: // Def
        yylhs.value.emplace< std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > ();
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        yylhs.value.emplace< std::tuple<TypePtr, std::string> > ();
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        yylhs.value.emplace< std::vector<ast::ExprPtr> > ();
        break;

      case symbol_kind::S_DefList: // DefList
        yylhs.value.emplace< std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ();
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        yylhs.value.emplace< std::vector<std::tuple<TypePtr, std::string>> > ();
        break;

      default:
        break;
    }


      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 3: // Stmts: Stmt
#line 89 "frontend/parser.y"
         {
    if (yystack_[0].value.as < ast::StmtPtr > () != nullptr) {
      driver.add_stmt(yystack_[0].value.as < ast::StmtPtr > ());
    }
  }
#line 867 "frontend/generated/parser.cpp"
    break;

  case 4: // Stmts: Stmts Stmt
#line 94 "frontend/parser.y"
               {
    if (yystack_[0].value.as < ast::StmtPtr > () != nullptr) {
      driver.add_stmt(yystack_[0].value.as < ast::StmtPtr > ());
    }
  }
#line 877 "frontend/generated/parser.cpp"
    break;

  case 5: // Stmt: AssignStmt
#line 102 "frontend/parser.y"
               {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 885 "frontend/generated/parser.cpp"
    break;

  case 6: // Stmt: IfStmt
#line 105 "frontend/parser.y"
           {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 893 "frontend/generated/parser.cpp"
    break;

  case 7: // Stmt: WhileStmt
#line 108 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 901 "frontend/generated/parser.cpp"
    break;

  case 8: // Stmt: ReturnStmt
#line 111 "frontend/parser.y"
               {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 909 "frontend/generated/parser.cpp"
    break;

  case 9: // Stmt: ContinueStmt
#line 114 "frontend/parser.y"
                 {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 917 "frontend/generated/parser.cpp"
    break;

  case 10: // Stmt: BreakStmt
#line 117 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 925 "frontend/generated/parser.cpp"
    break;

  case 11: // Stmt: DeclStmt
#line 120 "frontend/parser.y"
             {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 933 "frontend/generated/parser.cpp"
    break;

  case 12: // Stmt: ExprStmt
#line 123 "frontend/parser.y"
             {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 941 "frontend/generated/parser.cpp"
    break;

  case 13: // Stmt: BlankStmt
#line 126 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 949 "frontend/generated/parser.cpp"
    break;

  case 14: // Stmt: BlockStmt
#line 129 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = yystack_[0].value.as < ast::StmtPtr > ();
  }
#line 957 "frontend/generated/parser.cpp"
    break;

  case 15: // Stmt: FuncDef
#line 132 "frontend/parser.y"
            {
    yylhs.value.as < ast::StmtPtr > () = nullptr;
  }
#line 965 "frontend/generated/parser.cpp"
    break;

  case 16: // $@1: %empty
#line 138 "frontend/parser.y"
                            {
    driver.add_function(yystack_[3].value.as < TypePtr > (), yystack_[2].value.as < std::string > (), {});
  }
#line 973 "frontend/generated/parser.cpp"
    break;

  case 17: // FuncDef: Type IDENTIFIER '(' ')' $@1 BlockStmt
#line 140 "frontend/parser.y"
              {
    driver.quit_function();
  }
#line 981 "frontend/generated/parser.cpp"
    break;

  case 18: // $@2: %empty
#line 143 "frontend/parser.y"
                                          {
    driver.add_function(yystack_[4].value.as < TypePtr > (), yystack_[3].value.as < std::string > (), yystack_[1].value.as < std::vector<std::tuple<TypePtr, std::string>> > ());
  }
#line 989 "frontend/generated/parser.cpp"
    break;

  case 19: // FuncDef: Type IDENTIFIER '(' FuncParamList ')' $@2 BlockStmt
#line 145 "frontend/parser.y"
              {
    driver.quit_function();
  }
#line 997 "frontend/generated/parser.cpp"
    break;

  case 20: // ExprStmt: Expr ';'
#line 151 "frontend/parser.y"
             {
    yylhs.value.as < ast::StmtPtr > () = ast::create_expr_stmt(yystack_[1].value.as < ast::ExprPtr > ());
  }
#line 1005 "frontend/generated/parser.cpp"
    break;

  case 21: // DeclStmt: Type DefList ';'
#line 157 "frontend/parser.y"
                     {
    yylhs.value.as < ast::StmtPtr > () = ast::create_decl_stmt(driver.curr_decl_scope, false, yystack_[1].value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ());
  }
#line 1013 "frontend/generated/parser.cpp"
    break;

  case 22: // DeclStmt: CONST Type DefList ';'
#line 160 "frontend/parser.y"
                           {
    yylhs.value.as < ast::StmtPtr > () = ast::create_decl_stmt(driver.curr_decl_scope, true, yystack_[1].value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ());
    driver.is_curr_decl_const = false;
  }
#line 1022 "frontend/generated/parser.cpp"
    break;

  case 23: // DefList: DefList ',' Def
#line 167 "frontend/parser.y"
                    {
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > () = yystack_[2].value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ();
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ().push_back(yystack_[0].value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > ());

    auto symbol_entry = ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      yystack_[0].value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > ()
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
#line 1038 "frontend/generated/parser.cpp"
    break;

  case 24: // DefList: Def
#line 178 "frontend/parser.y"
        {
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>>> > ().push_back(yystack_[0].value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > ());

    auto symbol_entry = ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      yystack_[0].value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > ()
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
#line 1053 "frontend/generated/parser.cpp"
    break;

  case 25: // Def: IDENTIFIER
#line 191 "frontend/parser.y"
               {
    yylhs.value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > () = std::make_tuple(driver.curr_decl_type, yystack_[0].value.as < std::string > (), std::nullopt);
  }
#line 1061 "frontend/generated/parser.cpp"
    break;

  case 26: // Def: IDENTIFIER ArrayIndices
#line 194 "frontend/parser.y"
                            {
    TypePtr decl_type = driver.curr_decl_type;

    auto curr_index_expr = yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rbegin();
    while (curr_index_expr != yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rend()) {
      auto maybe_decl_type = create_array_type_from_expr(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << yystack_[0].location << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    yylhs.value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > () = std::make_tuple(decl_type, yystack_[1].value.as < std::string > (), std::nullopt);
  }
#line 1085 "frontend/generated/parser.cpp"
    break;

  case 27: // Def: IDENTIFIER '=' InitVal
#line 213 "frontend/parser.y"
                           {
    auto init_val = yystack_[0].value.as < ast::ExprPtr > ();
    if (init_val->is_initializer_list()) {
      std::get<ast::expr::InitializerList>(init_val->kind)
        .set_type(driver.curr_decl_type);
    }
    yylhs.value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > () = std::make_tuple(
      driver.curr_decl_type, 
      yystack_[2].value.as < std::string > (), 
      std::make_optional(init_val)
    );
  }
#line 1102 "frontend/generated/parser.cpp"
    break;

  case 28: // Def: IDENTIFIER ArrayIndices '=' InitVal
#line 225 "frontend/parser.y"
                                        {
    TypePtr decl_type = driver.curr_decl_type;

    auto curr_index_expr = yystack_[2].value.as < std::vector<ast::ExprPtr> > ().rbegin();
    while (curr_index_expr != yystack_[2].value.as < std::vector<ast::ExprPtr> > ().rend()) {
      auto maybe_decl_type = create_array_type_from_expr(
        decl_type, *curr_index_expr);
      if (!maybe_decl_type.has_value()) {
        std::cerr << yystack_[2].location << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      decl_type = maybe_decl_type.value();
      ++curr_index_expr;
    }

    auto init_val = yystack_[0].value.as < ast::ExprPtr > ();
    if (init_val->is_initializer_list()) {
      std::get<ast::expr::InitializerList>(init_val->kind).set_type(decl_type);
    }
    yylhs.value.as < std::tuple<TypePtr, std::string, std::optional<ast::ExprPtr>> > () = std::make_tuple(decl_type, yystack_[3].value.as < std::string > (), std::make_optional(init_val));
  }
#line 1130 "frontend/generated/parser.cpp"
    break;

  case 29: // ArrayIndices: '[' Expr ']'
#line 251 "frontend/parser.y"
                 {
    yylhs.value.as < std::vector<ast::ExprPtr> > () = { yystack_[1].value.as < ast::ExprPtr > () };
  }
#line 1138 "frontend/generated/parser.cpp"
    break;

  case 30: // ArrayIndices: ArrayIndices '[' Expr ']'
#line 254 "frontend/parser.y"
                              {
    yylhs.value.as < std::vector<ast::ExprPtr> > () = yystack_[3].value.as < std::vector<ast::ExprPtr> > ();
    yylhs.value.as < std::vector<ast::ExprPtr> > ().push_back(yystack_[1].value.as < ast::ExprPtr > ());
  }
#line 1147 "frontend/generated/parser.cpp"
    break;

  case 31: // InitVal: Expr
#line 261 "frontend/parser.y"
         {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1155 "frontend/generated/parser.cpp"
    break;

  case 32: // InitVal: '{' '}'
#line 264 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = ast::create_initializer_list_expr({});
  }
#line 1163 "frontend/generated/parser.cpp"
    break;

  case 33: // InitVal: '{' InitValList '}'
#line 267 "frontend/parser.y"
                        {
    yylhs.value.as < ast::ExprPtr > () = ast::create_initializer_list_expr(yystack_[1].value.as < std::vector<ast::ExprPtr> > ());
  }
#line 1171 "frontend/generated/parser.cpp"
    break;

  case 34: // InitValList: InitVal
#line 273 "frontend/parser.y"
            {
    yylhs.value.as < std::vector<ast::ExprPtr> > ().push_back(yystack_[0].value.as < ast::ExprPtr > ());
  }
#line 1179 "frontend/generated/parser.cpp"
    break;

  case 35: // InitValList: InitValList ',' InitVal
#line 276 "frontend/parser.y"
                            {
    yylhs.value.as < std::vector<ast::ExprPtr> > () = yystack_[2].value.as < std::vector<ast::ExprPtr> > ();
    yylhs.value.as < std::vector<ast::ExprPtr> > ().push_back(yystack_[0].value.as < ast::ExprPtr > ());
  }
#line 1188 "frontend/generated/parser.cpp"
    break;

  case 36: // IfStmt: IF '(' Cond ')' Stmt
#line 283 "frontend/parser.y"
                                    {
    yylhs.value.as < ast::StmtPtr > () = ast::create_if_stmt(yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::StmtPtr > (), std::nullopt);
  }
#line 1196 "frontend/generated/parser.cpp"
    break;

  case 37: // IfStmt: IF '(' Cond ')' Stmt ELSE Stmt
#line 286 "frontend/parser.y"
                                   {
    yylhs.value.as < ast::StmtPtr > () = ast::create_if_stmt(yystack_[4].value.as < ast::ExprPtr > (), yystack_[2].value.as < ast::StmtPtr > (), std::make_optional(yystack_[0].value.as < ast::StmtPtr > ()));
  }
#line 1204 "frontend/generated/parser.cpp"
    break;

  case 38: // WhileStmt: WHILE '(' Cond ')' Stmt
#line 292 "frontend/parser.y"
                            {
    yylhs.value.as < ast::StmtPtr > () = ast::create_while_stmt(yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::StmtPtr > ());
  }
#line 1212 "frontend/generated/parser.cpp"
    break;

  case 39: // ReturnStmt: RETURN Expr ';'
#line 298 "frontend/parser.y"
                    {
    yylhs.value.as < ast::StmtPtr > () = ast::create_return_stmt(std::make_optional(yystack_[1].value.as < ast::ExprPtr > ()));
  }
#line 1220 "frontend/generated/parser.cpp"
    break;

  case 40: // ReturnStmt: RETURN ';'
#line 301 "frontend/parser.y"
               {
    yylhs.value.as < ast::StmtPtr > () = ast::create_return_stmt(std::nullopt);
  }
#line 1228 "frontend/generated/parser.cpp"
    break;

  case 41: // ContinueStmt: CONTINUE ';'
#line 307 "frontend/parser.y"
                 {
    yylhs.value.as < ast::StmtPtr > () = ast::create_continue_stmt();
  }
#line 1236 "frontend/generated/parser.cpp"
    break;

  case 42: // BreakStmt: BREAK ';'
#line 313 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = ast::create_break_stmt();
  }
#line 1244 "frontend/generated/parser.cpp"
    break;

  case 43: // $@3: %empty
#line 319 "frontend/parser.y"
        {
    driver.add_block();
  }
#line 1252 "frontend/generated/parser.cpp"
    break;

  case 44: // BlockStmt: '{' $@3 Stmts '}'
#line 321 "frontend/parser.y"
              {
    yylhs.value.as < ast::StmtPtr > () = driver.curr_block;
    driver.quit_block();
  }
#line 1261 "frontend/generated/parser.cpp"
    break;

  case 45: // BlockStmt: '{' '}'
#line 325 "frontend/parser.y"
            {
    // Just ignore.
    yylhs.value.as < ast::StmtPtr > () = ast::create_blank_stmt();
  }
#line 1270 "frontend/generated/parser.cpp"
    break;

  case 46: // BlankStmt: ';'
#line 332 "frontend/parser.y"
        {
    yylhs.value.as < ast::StmtPtr > () = ast::create_blank_stmt();
  }
#line 1278 "frontend/generated/parser.cpp"
    break;

  case 47: // Expr: AddExpr
#line 339 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1286 "frontend/generated/parser.cpp"
    break;

  case 48: // Cond: LOrExpr
#line 345 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1294 "frontend/generated/parser.cpp"
    break;

  case 49: // PrimaryExpr: '(' Expr ')'
#line 351 "frontend/parser.y"
                 {
    yylhs.value.as < ast::ExprPtr > () = yystack_[1].value.as < ast::ExprPtr > ();
  }
#line 1302 "frontend/generated/parser.cpp"
    break;

  case 50: // PrimaryExpr: LVal
#line 354 "frontend/parser.y"
         {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1310 "frontend/generated/parser.cpp"
    break;

  case 51: // PrimaryExpr: INTEGER
#line 357 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = ast::create_constant_expr(yystack_[0].value.as < ComptimeValue > ());
  }
#line 1318 "frontend/generated/parser.cpp"
    break;

  case 52: // PrimaryExpr: FLOATING
#line 360 "frontend/parser.y"
             {
    yylhs.value.as < ast::ExprPtr > () = ast::create_constant_expr(yystack_[0].value.as < ComptimeValue > ());
  }
#line 1326 "frontend/generated/parser.cpp"
    break;

  case 53: // LVal: IDENTIFIER
#line 366 "frontend/parser.y"
               {
    auto maybe_symbol_entry = driver.curr_symtable->lookup(yystack_[0].value.as < std::string > ());
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << yystack_[0].location << ":" << "Undefined identifier: " + yystack_[0].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < ast::ExprPtr > () = ast::create_identifier_expr(maybe_symbol_entry.value());
  }
#line 1339 "frontend/generated/parser.cpp"
    break;

  case 54: // LVal: LVal '[' Expr ']'
#line 374 "frontend/parser.y"
                      {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Index, yystack_[3].value.as < ast::ExprPtr > (), yystack_[1].value.as < ast::ExprPtr > (), driver
    );
  }
#line 1349 "frontend/generated/parser.cpp"
    break;

  case 55: // AssignStmt: LVal '=' Expr ';'
#line 382 "frontend/parser.y"
                      {
    yylhs.value.as < ast::StmtPtr > () = ast::create_assign_stmt(yystack_[3].value.as < ast::ExprPtr > (), yystack_[1].value.as < ast::ExprPtr > ());
  }
#line 1357 "frontend/generated/parser.cpp"
    break;

  case 56: // UnaryExpr: PrimaryExpr
#line 388 "frontend/parser.y"
                {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1365 "frontend/generated/parser.cpp"
    break;

  case 57: // UnaryExpr: '+' UnaryExpr
#line 391 "frontend/parser.y"
                  {
    yylhs.value.as < ast::ExprPtr > () = ast::create_unary_expr(UnaryOp::Pos, yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1373 "frontend/generated/parser.cpp"
    break;

  case 58: // UnaryExpr: '-' UnaryExpr
#line 394 "frontend/parser.y"
                  {
    yylhs.value.as < ast::ExprPtr > () = ast::create_unary_expr(UnaryOp::Neg, yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1381 "frontend/generated/parser.cpp"
    break;

  case 59: // UnaryExpr: '!' UnaryExpr
#line 397 "frontend/parser.y"
                  {
    yylhs.value.as < ast::ExprPtr > () = ast::create_unary_expr(UnaryOp::LogicalNot, yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1389 "frontend/generated/parser.cpp"
    break;

  case 60: // UnaryExpr: IDENTIFIER '(' FuncArgList ')'
#line 400 "frontend/parser.y"
                                   {
    auto maybe_symbol_entry = driver.compunit.symtable->lookup(yystack_[3].value.as < std::string > ());
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << yystack_[3].location << ":" << "Undefined identifier: " + yystack_[3].value.as < std::string > ();
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << yystack_[3].location << ":" << "Not a function: " + yystack_[3].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < ast::ExprPtr > () = ast::create_call_expr(
      maybe_symbol_entry.value(), 
      yystack_[1].value.as < std::vector<ast::ExprPtr> > (), 
      driver
    );
  }
#line 1413 "frontend/generated/parser.cpp"
    break;

  case 61: // UnaryExpr: IDENTIFIER '(' ')'
#line 419 "frontend/parser.y"
                       {
    if (yystack_[2].value.as < std::string > () == "starttime" || yystack_[2].value.as < std::string > () == "stoptime") {
      auto maybe_symbol_entry = driver.compunit.symtable->lookup("_sysy_" + yystack_[2].value.as < std::string > ());
      int lineno = yystack_[2].location.end.line;
      auto lineno_expr = ast::create_constant_expr(
        create_comptime_value(lineno, create_int_type()));
      yylhs.value.as < ast::ExprPtr > () = ast::create_call_expr(
        maybe_symbol_entry.value(), 
        {lineno_expr}, 
        driver
      );
      YYACCEPT;
    }
    auto maybe_symbol_entry = driver.compunit.symtable->lookup(yystack_[2].value.as < std::string > ());
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << yystack_[2].location << ":" << "Undefined identifier: " + yystack_[2].value.as < std::string > ();
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << yystack_[2].location << ":" << "Not a function: " + yystack_[2].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < ast::ExprPtr > () = ast::create_call_expr(
      maybe_symbol_entry.value(), 
      {}, 
      driver
    );
  }
#line 1449 "frontend/generated/parser.cpp"
    break;

  case 62: // MulExpr: UnaryExpr
#line 453 "frontend/parser.y"
              {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1457 "frontend/generated/parser.cpp"
    break;

  case 63: // MulExpr: MulExpr '*' UnaryExpr
#line 456 "frontend/parser.y"
                          {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Mul, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1466 "frontend/generated/parser.cpp"
    break;

  case 64: // MulExpr: MulExpr '/' UnaryExpr
#line 460 "frontend/parser.y"
                          {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Div, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1475 "frontend/generated/parser.cpp"
    break;

  case 65: // MulExpr: MulExpr '%' UnaryExpr
#line 464 "frontend/parser.y"
                          {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Mod, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1484 "frontend/generated/parser.cpp"
    break;

  case 66: // AddExpr: MulExpr
#line 471 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1492 "frontend/generated/parser.cpp"
    break;

  case 67: // AddExpr: AddExpr '+' MulExpr
#line 474 "frontend/parser.y"
                        {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Add, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1501 "frontend/generated/parser.cpp"
    break;

  case 68: // AddExpr: AddExpr '-' MulExpr
#line 478 "frontend/parser.y"
                        {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Sub, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1510 "frontend/generated/parser.cpp"
    break;

  case 69: // RelExpr: AddExpr
#line 485 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1518 "frontend/generated/parser.cpp"
    break;

  case 70: // RelExpr: RelExpr '<' AddExpr
#line 488 "frontend/parser.y"
                        {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Lt, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1527 "frontend/generated/parser.cpp"
    break;

  case 71: // RelExpr: RelExpr '>' AddExpr
#line 492 "frontend/parser.y"
                        {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Gt, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1536 "frontend/generated/parser.cpp"
    break;

  case 72: // RelExpr: RelExpr LE AddExpr
#line 496 "frontend/parser.y"
                       {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Le, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1545 "frontend/generated/parser.cpp"
    break;

  case 73: // RelExpr: RelExpr GE AddExpr
#line 500 "frontend/parser.y"
                       {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Ge, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1554 "frontend/generated/parser.cpp"
    break;

  case 74: // EqExpr: RelExpr
#line 507 "frontend/parser.y"
            {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1562 "frontend/generated/parser.cpp"
    break;

  case 75: // EqExpr: EqExpr EQ RelExpr
#line 510 "frontend/parser.y"
                      {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Eq, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1571 "frontend/generated/parser.cpp"
    break;

  case 76: // EqExpr: EqExpr NE RelExpr
#line 514 "frontend/parser.y"
                      {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::Ne, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1580 "frontend/generated/parser.cpp"
    break;

  case 77: // LAndExpr: EqExpr
#line 521 "frontend/parser.y"
           {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1588 "frontend/generated/parser.cpp"
    break;

  case 78: // LAndExpr: LAndExpr LAND EqExpr
#line 524 "frontend/parser.y"
                         {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::LogicalAnd, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1597 "frontend/generated/parser.cpp"
    break;

  case 79: // LOrExpr: LAndExpr
#line 531 "frontend/parser.y"
             {
    yylhs.value.as < ast::ExprPtr > () = yystack_[0].value.as < ast::ExprPtr > ();
  }
#line 1605 "frontend/generated/parser.cpp"
    break;

  case 80: // LOrExpr: LOrExpr LOR LAndExpr
#line 534 "frontend/parser.y"
                         {
    yylhs.value.as < ast::ExprPtr > () = ast::create_binary_expr(
      BinaryOp::LogicalOr, yystack_[2].value.as < ast::ExprPtr > (), yystack_[0].value.as < ast::ExprPtr > (), driver);
  }
#line 1614 "frontend/generated/parser.cpp"
    break;

  case 81: // FuncArgList: Expr
#line 541 "frontend/parser.y"
         {
    yylhs.value.as < std::vector<ast::ExprPtr> > ().push_back(yystack_[0].value.as < ast::ExprPtr > ());
  }
#line 1622 "frontend/generated/parser.cpp"
    break;

  case 82: // FuncArgList: FuncArgList ',' Expr
#line 544 "frontend/parser.y"
                         {
    yylhs.value.as < std::vector<ast::ExprPtr> > () = yystack_[2].value.as < std::vector<ast::ExprPtr> > ();
    yylhs.value.as < std::vector<ast::ExprPtr> > ().push_back(yystack_[0].value.as < ast::ExprPtr > ());
  }
#line 1631 "frontend/generated/parser.cpp"
    break;

  case 83: // FuncParamList: FuncParam
#line 551 "frontend/parser.y"
              {
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string>> > ().push_back(yystack_[0].value.as < std::tuple<TypePtr, std::string> > ());
  }
#line 1639 "frontend/generated/parser.cpp"
    break;

  case 84: // FuncParamList: FuncParamList ',' FuncParam
#line 554 "frontend/parser.y"
                                {
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string>> > () = yystack_[2].value.as < std::vector<std::tuple<TypePtr, std::string>> > ();
    yylhs.value.as < std::vector<std::tuple<TypePtr, std::string>> > ().push_back(yystack_[0].value.as < std::tuple<TypePtr, std::string> > ());
  }
#line 1648 "frontend/generated/parser.cpp"
    break;

  case 85: // FuncParam: Type IDENTIFIER
#line 561 "frontend/parser.y"
                    {
    yylhs.value.as < std::tuple<TypePtr, std::string> > () = std::make_tuple(yystack_[1].value.as < TypePtr > (), yystack_[0].value.as < std::string > ());
  }
#line 1656 "frontend/generated/parser.cpp"
    break;

  case 86: // FuncParam: Type IDENTIFIER ArrayIndices
#line 564 "frontend/parser.y"
                                 {
    TypePtr type = yystack_[2].value.as < TypePtr > ();

    auto curr_index_expr = yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rbegin();
    while (curr_index_expr != yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rend()) {
      auto maybe_type = create_array_type_from_expr(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << yystack_[1].location << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    yylhs.value.as < std::tuple<TypePtr, std::string> > () = std::make_tuple(type, yystack_[1].value.as < std::string > ());
  }
#line 1680 "frontend/generated/parser.cpp"
    break;

  case 87: // FuncParam: Type IDENTIFIER '[' ']' ArrayIndices
#line 583 "frontend/parser.y"
                                         {
    TypePtr type = yystack_[4].value.as < TypePtr > ();

    auto curr_index_expr = yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rbegin();
    while (curr_index_expr != yystack_[0].value.as < std::vector<ast::ExprPtr> > ().rend()) {
      auto maybe_type = create_array_type_from_expr(
        type, *curr_index_expr);
      if (!maybe_type.has_value()) {
        std::cerr << yystack_[3].location << ":"
                  << "Array length must be compile-time available value" 
                  << std::endl;
        YYABORT;
      }
      type = maybe_type.value();
      ++curr_index_expr;
    }

    type = create_array_type(type, std::nullopt);

    yylhs.value.as < std::tuple<TypePtr, std::string> > () = std::make_tuple(type, yystack_[3].value.as < std::string > ());
  }
#line 1706 "frontend/generated/parser.cpp"
    break;

  case 88: // Type: INT
#line 607 "frontend/parser.y"
        {
    yylhs.value.as < TypePtr > () = create_int_type();
    driver.curr_decl_type = yylhs.value.as < TypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;

  }
#line 1718 "frontend/generated/parser.cpp"
    break;

  case 89: // Type: FLOAT
#line 614 "frontend/parser.y"
          {
    yylhs.value.as < TypePtr > () = create_float_type();
    driver.curr_decl_type = yylhs.value.as < TypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;
  }
#line 1729 "frontend/generated/parser.cpp"
    break;

  case 90: // Type: VOID
#line 620 "frontend/parser.y"
         {
    yylhs.value.as < TypePtr > () = create_void_type();
    driver.curr_decl_type = yylhs.value.as < TypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? Scope::Global 
                                                     : Scope::Local;
  }
#line 1740 "frontend/generated/parser.cpp"
    break;


#line 1744 "frontend/generated/parser.cpp"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        context yyctx (*this, yyla);
        std::string msg = yysyntax_error_ (yyctx);
        error (yyla.location, YY_MOVE (msg));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.kind () == symbol_kind::S_YYEOF)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    YY_STACK_PRINT ();
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    // Pop stack until we find a state that shifts the error token.
    for (;;)
      {
        yyn = yypact_[+yystack_[0].state];
        if (!yy_pact_value_is_default_ (yyn))
          {
            yyn += symbol_kind::S_YYerror;
            if (0 <= yyn && yyn <= yylast_
                && yycheck_[yyn] == symbol_kind::S_YYerror)
              {
                yyn = yytable_[yyn];
                if (0 < yyn)
                  break;
              }
          }

        // Pop the current state because it cannot handle the error token.
        if (yystack_.size () == 1)
          YYABORT;

        yyerror_range[1].location = yystack_[0].location;
        yy_destroy_ ("Error: popping", yystack_[0]);
        yypop_ ();
        YY_STACK_PRINT ();
      }
    {
      stack_symbol_type error_token;

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    YY_STACK_PRINT ();
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
   Parser ::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
   Parser ::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }

  std::string
   Parser ::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  //  Parser ::context.
   Parser ::context::context (const  Parser & yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
   Parser ::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
  {
    // Actual number of expected tokens
    int yycount = 0;

    const int yyn = yypact_[+yyparser_.yystack_[0].state];
    if (!yy_pact_value_is_default_ (yyn))
      {
        /* Start YYX at -YYN if negative to avoid negative indexes in
           YYCHECK.  In other words, skip the first -YYN actions for
           this state because they are default actions.  */
        const int yyxbegin = yyn < 0 ? -yyn : 0;
        // Stay within bounds of both yycheck and yytname.
        const int yychecklim = yylast_ - yyn + 1;
        const int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
        for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
          if (yycheck_[yyx + yyn] == yyx && yyx != symbol_kind::S_YYerror
              && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
            {
              if (!yyarg)
                ++yycount;
              else if (yycount == yyargn)
                return 0;
              else
                yyarg[yycount++] = YY_CAST (symbol_kind_type, yyx);
            }
      }

    if (yyarg && yycount == 0 && 0 < yyargn)
      yyarg[0] = symbol_kind::S_YYEMPTY;
    return yycount;
  }






  int
   Parser ::yy_syntax_error_arguments_ (const context& yyctx,
                                                 symbol_kind_type yyarg[], int yyargn) const
  {
    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */

    if (!yyctx.lookahead ().empty ())
      {
        if (yyarg)
          yyarg[0] = yyctx.token ();
        int yyn = yyctx.expected_tokens (yyarg ? yyarg + 1 : yyarg, yyargn - 1);
        return yyn + 1;
      }
    return 0;
  }

  // Generate an error message.
  std::string
   Parser ::yysyntax_error_ (const context& yyctx) const
  {
    // Its maximum.
    enum { YYARGS_MAX = 5 };
    // Arguments of yyformat.
    symbol_kind_type yyarg[YYARGS_MAX];
    int yycount = yy_syntax_error_arguments_ (yyctx, yyarg, YYARGS_MAX);

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += symbol_name (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const signed char  Parser ::yypact_ninf_ = -124;

  const signed char  Parser ::yytable_ninf_ = -1;

  const short
   Parser ::yypact_[] =
  {
     171,   205,   205,   205,     5,   205,  -124,    13,  -124,  -124,
      40,    50,    17,    58,    63,   -24,  -124,  -124,  -124,    93,
     171,  -124,  -124,  -124,  -124,  -124,  -124,  -124,  -124,  -124,
    -124,  -124,    66,  -124,    89,  -124,  -124,    67,    86,    62,
      95,  -124,  -124,    96,  -124,   171,  -124,    42,   205,   205,
    -124,   104,  -124,  -124,   100,  -124,  -124,  -124,   205,   205,
     205,   205,   205,   205,   205,    51,    83,  -124,  -124,   137,
    -124,  -124,     1,   113,    86,    31,    82,   116,   114,   130,
    -124,   103,    97,   136,   145,  -124,  -124,  -124,    67,    67,
      81,   205,    -5,   107,   100,  -124,  -124,  -124,   205,   171,
     205,   205,   205,   205,   205,   205,   205,   205,   171,  -124,
    -124,  -124,    12,  -124,  -124,   146,  -124,    23,  -124,   133,
      81,   205,  -124,  -124,   148,    86,    86,    86,    86,    31,
      31,    82,   116,  -124,  -124,  -124,    65,  -124,   152,  -124,
     -24,   168,  -124,   169,   171,  -124,    81,  -124,   152,  -124,
     134,   172,  -124,  -124,  -124,  -124,   174,   172
  };

  const signed char
   Parser ::yydefact_[] =
  {
       0,     0,     0,     0,    43,     0,    46,    53,    51,    52,
       0,     0,     0,     0,     0,     0,    88,    89,    90,     0,
       2,     3,    15,    12,    11,     6,     7,     8,     9,    10,
      14,    13,     0,    56,    50,     5,    62,    66,    47,     0,
      50,    57,    58,     0,    45,     0,    59,     0,     0,     0,
      40,     0,    42,    41,     0,     1,     4,    20,     0,     0,
       0,     0,     0,     0,     0,    25,     0,    24,    49,     0,
      61,    81,     0,     0,    69,    74,    77,    79,    48,     0,
      39,    25,     0,     0,     0,    63,    64,    65,    67,    68,
       0,     0,     0,    26,     0,    21,    44,    60,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    22,
      55,    54,     0,    27,    31,     0,    16,     0,    83,     0,
       0,     0,    23,    82,    36,    72,    73,    70,    71,    75,
      76,    78,    80,    38,    32,    34,     0,    29,     0,    18,
       0,    85,    28,     0,     0,    33,     0,    17,     0,    84,
       0,    86,    30,    37,    35,    19,     0,    87
  };

  const short
   Parser ::yypgoto_[] =
  {
    -124,  -124,   135,   -17,  -124,  -124,  -124,  -124,  -124,   124,
      91,  -123,  -106,  -124,  -124,  -124,  -124,  -124,  -124,   -90,
    -124,  -124,    -3,   138,  -124,     2,  -124,     3,    56,    28,
      18,    84,    85,  -124,  -124,  -124,    49,   -14
  };

  const unsigned char
   Parser ::yydefgoto_[] =
  {
       0,    19,    20,    21,    22,   138,   148,    23,    24,    66,
      67,    93,   113,   136,    25,    26,    27,    28,    29,    30,
      45,    31,    32,    73,    33,    40,    35,    36,    37,    38,
      75,    76,    77,    78,    72,   117,   118,    39
  };

  const unsigned char
   Parser ::yytable_[] =
  {
      43,    54,    34,    56,    41,    42,   135,   116,    46,    51,
      16,    17,    18,    97,   142,     1,     2,    98,   151,    44,
       1,     2,    34,     3,    47,   112,   134,     5,     3,    16,
      17,    18,     5,   157,    50,   139,     7,     8,     9,   140,
     154,     7,     8,     9,    71,     1,     2,    34,   147,   100,
     101,    48,    56,     3,    70,    83,    84,     5,   155,    90,
      91,    49,    92,    85,    86,    87,     7,     8,     9,   102,
     103,    34,    60,    61,    62,    52,    74,    74,   119,   145,
      53,   146,   124,    57,     1,     2,    65,   114,   115,    63,
      64,   133,     3,    55,   112,   123,     5,    58,    59,    94,
      95,    34,   104,   105,    59,     7,     8,     9,    68,   114,
      34,    90,    91,    94,   109,   120,   121,   114,   143,    88,
      89,    80,   129,   130,    81,    99,   119,   153,   125,   126,
     127,   128,    74,    74,    74,    74,   107,     1,     2,   106,
       1,     2,   108,   114,   156,     3,    34,   115,     3,     5,
       4,    96,     5,   110,     6,   111,   137,   141,     7,     8,
       9,     7,     8,     9,    10,     4,    11,    12,    13,    14,
      15,    16,    17,    18,     1,     2,   144,   150,    82,   152,
      69,   121,     3,    91,     4,   122,     5,    79,     6,   149,
     131,     0,   132,     0,     0,     7,     8,     9,    10,     0,
      11,    12,    13,    14,    15,    16,    17,    18,     1,     2,
       0,     0,     0,     0,     0,     0,     3,     0,     0,     0,
       5,     0,     0,     0,     0,     0,     0,     0,     0,     7,
       8,     9
  };

  const short
   Parser ::yycheck_[] =
  {
       3,    15,     0,    20,     1,     2,   112,    12,     5,    12,
      34,    35,    36,    12,   120,     3,     4,    16,   141,    14,
       3,     4,    20,    11,    11,    13,    14,    15,    11,    34,
      35,    36,    15,   156,    17,    12,    24,    25,    26,    16,
     146,    24,    25,    26,    47,     3,     4,    45,   138,    18,
      19,    11,    69,    11,    12,    58,    59,    15,   148,     8,
       9,    11,    11,    60,    61,    62,    24,    25,    26,    38,
      39,    69,     5,     6,     7,    17,    48,    49,    92,    14,
      17,    16,    99,    17,     3,     4,    24,    90,    91,     3,
       4,   108,    11,     0,    13,    98,    15,     8,     9,    16,
      17,    99,    20,    21,     9,    24,    25,    26,    12,   112,
     108,     8,     9,    16,    17,     8,     9,   120,   121,    63,
      64,    17,   104,   105,    24,    12,   140,   144,   100,   101,
     102,   103,   104,   105,   106,   107,    22,     3,     4,    23,
       3,     4,    12,   146,    10,    11,   144,   150,    11,    15,
      13,    14,    15,    17,    17,    10,    10,    24,    24,    25,
      26,    24,    25,    26,    27,    13,    29,    30,    31,    32,
      33,    34,    35,    36,     3,     4,    28,     9,    54,    10,
      45,     9,    11,     9,    13,    94,    15,    49,    17,   140,
     106,    -1,   107,    -1,    -1,    24,    25,    26,    27,    -1,
      29,    30,    31,    32,    33,    34,    35,    36,     3,     4,
      -1,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,    -1,
      15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    24,
      25,    26
  };

  const signed char
   Parser ::yystos_[] =
  {
       0,     3,     4,    11,    13,    15,    17,    24,    25,    26,
      27,    29,    30,    31,    32,    33,    34,    35,    36,    41,
      42,    43,    44,    47,    48,    54,    55,    56,    57,    58,
      59,    61,    62,    64,    65,    66,    67,    68,    69,    77,
      65,    67,    67,    62,    14,    60,    67,    11,    11,    11,
      17,    62,    17,    17,    77,     0,    43,    17,     8,     9,
       5,     6,     7,     3,     4,    24,    49,    50,    12,    42,
      12,    62,    74,    63,    69,    70,    71,    72,    73,    63,
      17,    24,    49,    62,    62,    67,    67,    67,    68,    68,
       8,     9,    11,    51,    16,    17,    14,    12,    16,    12,
      18,    19,    38,    39,    20,    21,    23,    22,    12,    17,
      17,    10,    13,    52,    62,    62,    12,    75,    76,    77,
       8,     9,    50,    62,    43,    69,    69,    69,    69,    70,
      70,    71,    72,    43,    14,    52,    53,    10,    45,    12,
      16,    24,    52,    62,    28,    14,    16,    59,    46,    76,
       9,    51,    10,    43,    52,    59,    10,    51
  };

  const signed char
   Parser ::yyr1_[] =
  {
       0,    40,    41,    42,    42,    43,    43,    43,    43,    43,
      43,    43,    43,    43,    43,    43,    45,    44,    46,    44,
      47,    48,    48,    49,    49,    50,    50,    50,    50,    51,
      51,    52,    52,    52,    53,    53,    54,    54,    55,    56,
      56,    57,    58,    60,    59,    59,    61,    62,    63,    64,
      64,    64,    64,    65,    65,    66,    67,    67,    67,    67,
      67,    67,    68,    68,    68,    68,    69,    69,    69,    70,
      70,    70,    70,    70,    71,    71,    71,    72,    72,    73,
      73,    74,    74,    75,    75,    76,    76,    76,    77,    77,
      77
  };

  const signed char
   Parser ::yyr2_[] =
  {
       0,     2,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     6,     0,     7,
       2,     3,     4,     3,     1,     1,     2,     3,     4,     3,
       4,     1,     2,     3,     1,     3,     5,     7,     5,     3,
       2,     2,     2,     0,     4,     2,     1,     1,     1,     3,
       1,     1,     1,     1,     4,     4,     1,     2,     2,     2,
       4,     3,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     2,     3,     5,     1,     1,
       1
  };


#if YYDEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const  Parser ::yytname_[] =
  {
  "END", "error", "\"invalid token\"", "'+'", "'-'", "'*'", "'/'", "'%'",
  "'='", "'['", "']'", "'('", "')'", "'{'", "'}'", "'!'", "','", "';'",
  "LE", "GE", "EQ", "NE", "LOR", "LAND", "IDENTIFIER", "INTEGER",
  "FLOATING", "IF", "ELSE", "WHILE", "RETURN", "BREAK", "CONTINUE",
  "CONST", "INT", "FLOAT", "VOID", "THEN", "'<'", "'>'", "$accept",
  "Program", "Stmts", "Stmt", "FuncDef", "$@1", "$@2", "ExprStmt",
  "DeclStmt", "DefList", "Def", "ArrayIndices", "InitVal", "InitValList",
  "IfStmt", "WhileStmt", "ReturnStmt", "ContinueStmt", "BreakStmt",
  "BlockStmt", "$@3", "BlankStmt", "Expr", "Cond", "PrimaryExpr", "LVal",
  "AssignStmt", "UnaryExpr", "MulExpr", "AddExpr", "RelExpr", "EqExpr",
  "LAndExpr", "LOrExpr", "FuncArgList", "FuncParamList", "FuncParam",
  "Type", YY_NULLPTR
  };
#endif


#if YYDEBUG
  const short
   Parser ::yyrline_[] =
  {
       0,    85,    85,    89,    94,   102,   105,   108,   111,   114,
     117,   120,   123,   126,   129,   132,   138,   138,   143,   143,
     151,   157,   160,   167,   178,   191,   194,   213,   225,   251,
     254,   261,   264,   267,   273,   276,   283,   286,   292,   298,
     301,   307,   313,   319,   319,   325,   332,   339,   345,   351,
     354,   357,   360,   366,   374,   382,   388,   391,   394,   397,
     400,   419,   453,   456,   460,   464,   471,   474,   478,   485,
     488,   492,   496,   500,   507,   510,   514,   521,   524,   531,
     534,   541,   544,   551,   554,   561,   564,   583,   607,   614,
     620
  };

  void
   Parser ::yy_stack_print_ () const
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  void
   Parser ::yy_reduce_print_ (int yyrule) const
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG


#line 20 "frontend/parser.y"
} } //  syc::frontend 
#line 2338 "frontend/generated/parser.cpp"

#line 628 "frontend/parser.y"


namespace syc {
namespace frontend {

void Parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}

} // namespace frontend
} // namespace syc

// A Bison parser, made by GNU Bison 3.8.

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
#line 40 "frontend/parser.y"


  yy::parser::symbol_type yylex(
    yyscan_t yyscanner, 
    yy::location& loc, 
    frontend::Driver& driver
  );

#line 55 "frontend/generated/parser.cpp"


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

namespace yy {
#line 147 "frontend/generated/parser.cpp"

  /// Build a parser object.
  parser::parser (yyscan_t yyscanner_yyarg, yy::location& loc_yyarg, frontend::Driver& driver_yyarg)
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

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | symbol kinds.  |
  `---------------*/



  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_kind_type
  parser::by_state::kind () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return symbol_kind::S_YYEMPTY;
    else
      return YY_CAST (symbol_kind_type, yystos_[+state]);
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.YY_MOVE_OR_COPY< AstComptimeValue > (YY_MOVE (that.value));
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
        value.YY_MOVE_OR_COPY< AstExprPtr > (YY_MOVE (that.value));
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
        value.YY_MOVE_OR_COPY< AstStmtPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_Type: // Type
        value.YY_MOVE_OR_COPY< AstTypePtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.YY_MOVE_OR_COPY< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Def: // Def
        value.YY_MOVE_OR_COPY< std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.YY_MOVE_OR_COPY< std::tuple<AstTypePtr, std::string> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.YY_MOVE_OR_COPY< std::vector<AstExprPtr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DefList: // DefList
        value.YY_MOVE_OR_COPY< std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.YY_MOVE_OR_COPY< std::vector<std::tuple<AstTypePtr, std::string>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.location))
  {
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.move< AstComptimeValue > (YY_MOVE (that.value));
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
        value.move< AstExprPtr > (YY_MOVE (that.value));
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
        value.move< AstStmtPtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_Type: // Type
        value.move< AstTypePtr > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.move< std::string > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_Def: // Def
        value.move< std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.move< std::tuple<AstTypePtr, std::string> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.move< std::vector<AstExprPtr> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_DefList: // DefList
        value.move< std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > (YY_MOVE (that.value));
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.move< std::vector<std::tuple<AstTypePtr, std::string>> > (YY_MOVE (that.value));
        break;

      default:
        break;
    }

    // that is emptied.
    that.kind_ = symbol_kind::S_YYEMPTY;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.copy< AstComptimeValue > (that.value);
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
        value.copy< AstExprPtr > (that.value);
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
        value.copy< AstStmtPtr > (that.value);
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_Type: // Type
        value.copy< AstTypePtr > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.copy< std::string > (that.value);
        break;

      case symbol_kind::S_Def: // Def
        value.copy< std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > (that.value);
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.copy< std::tuple<AstTypePtr, std::string> > (that.value);
        break;

      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.copy< std::vector<AstExprPtr> > (that.value);
        break;

      case symbol_kind::S_DefList: // DefList
        value.copy< std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > (that.value);
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.copy< std::vector<std::tuple<AstTypePtr, std::string>> > (that.value);
        break;

      default:
        break;
    }

    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    switch (that.kind ())
    {
      case symbol_kind::S_INTEGER: // INTEGER
      case symbol_kind::S_FLOATING: // FLOATING
        value.move< AstComptimeValue > (that.value);
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
        value.move< AstExprPtr > (that.value);
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
        value.move< AstStmtPtr > (that.value);
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_Type: // Type
        value.move< AstTypePtr > (that.value);
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        value.move< std::string > (that.value);
        break;

      case symbol_kind::S_Def: // Def
        value.move< std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > (that.value);
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        value.move< std::tuple<AstTypePtr, std::string> > (that.value);
        break;

      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        value.move< std::vector<AstExprPtr> > (that.value);
        break;

      case symbol_kind::S_DefList: // DefList
        value.move< std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > (that.value);
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        value.move< std::vector<std::tuple<AstTypePtr, std::string>> > (that.value);
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
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo, const basic_symbol<Base>& yysym) const
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
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - YYNTOKENS] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - YYNTOKENS];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
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
        yylhs.value.emplace< AstComptimeValue > ();
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
        yylhs.value.emplace< AstExprPtr > ();
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
        yylhs.value.emplace< AstStmtPtr > ();
        break;

      case symbol_kind::S_ArrayIndices: // ArrayIndices
      case symbol_kind::S_Type: // Type
        yylhs.value.emplace< AstTypePtr > ();
        break;

      case symbol_kind::S_IDENTIFIER: // IDENTIFIER
        yylhs.value.emplace< std::string > ();
        break;

      case symbol_kind::S_Def: // Def
        yylhs.value.emplace< std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > ();
        break;

      case symbol_kind::S_FuncParam: // FuncParam
        yylhs.value.emplace< std::tuple<AstTypePtr, std::string> > ();
        break;

      case symbol_kind::S_InitValList: // InitValList
      case symbol_kind::S_FuncArgList: // FuncArgList
        yylhs.value.emplace< std::vector<AstExprPtr> > ();
        break;

      case symbol_kind::S_DefList: // DefList
        yylhs.value.emplace< std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ();
        break;

      case symbol_kind::S_FuncParamList: // FuncParamList
        yylhs.value.emplace< std::vector<std::tuple<AstTypePtr, std::string>> > ();
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
#line 93 "frontend/parser.y"
         {
    if (yystack_[0].value.as < AstStmtPtr > () != nullptr) {
      driver.add_stmt(yystack_[0].value.as < AstStmtPtr > ());
    }
  }
#line 867 "frontend/generated/parser.cpp"
    break;

  case 4: // Stmts: Stmts Stmt
#line 98 "frontend/parser.y"
               {
    if (yystack_[0].value.as < AstStmtPtr > () != nullptr) {
      driver.add_stmt(yystack_[0].value.as < AstStmtPtr > ());
    }
  }
#line 877 "frontend/generated/parser.cpp"
    break;

  case 5: // Stmt: AssignStmt
#line 106 "frontend/parser.y"
               {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 885 "frontend/generated/parser.cpp"
    break;

  case 6: // Stmt: IfStmt
#line 109 "frontend/parser.y"
           {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 893 "frontend/generated/parser.cpp"
    break;

  case 7: // Stmt: WhileStmt
#line 112 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 901 "frontend/generated/parser.cpp"
    break;

  case 8: // Stmt: ReturnStmt
#line 115 "frontend/parser.y"
               {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 909 "frontend/generated/parser.cpp"
    break;

  case 9: // Stmt: ContinueStmt
#line 118 "frontend/parser.y"
                 {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 917 "frontend/generated/parser.cpp"
    break;

  case 10: // Stmt: BreakStmt
#line 121 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 925 "frontend/generated/parser.cpp"
    break;

  case 11: // Stmt: DeclStmt
#line 124 "frontend/parser.y"
             {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 933 "frontend/generated/parser.cpp"
    break;

  case 12: // Stmt: ExprStmt
#line 127 "frontend/parser.y"
             {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 941 "frontend/generated/parser.cpp"
    break;

  case 13: // Stmt: BlankStmt
#line 130 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 949 "frontend/generated/parser.cpp"
    break;

  case 14: // Stmt: BlockStmt
#line 133 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = yystack_[0].value.as < AstStmtPtr > ();
  }
#line 957 "frontend/generated/parser.cpp"
    break;

  case 15: // Stmt: FuncDef
#line 136 "frontend/parser.y"
            {
    yylhs.value.as < AstStmtPtr > () = nullptr;
  }
#line 965 "frontend/generated/parser.cpp"
    break;

  case 16: // $@1: %empty
#line 142 "frontend/parser.y"
                            {
    driver.add_function(yystack_[3].value.as < AstTypePtr > (), yystack_[2].value.as < std::string > (), {});
  }
#line 973 "frontend/generated/parser.cpp"
    break;

  case 17: // FuncDef: Type IDENTIFIER '(' ')' $@1 BlockStmt
#line 144 "frontend/parser.y"
              {
    driver.quit_function();
  }
#line 981 "frontend/generated/parser.cpp"
    break;

  case 18: // $@2: %empty
#line 147 "frontend/parser.y"
                                          {
    driver.add_function(yystack_[4].value.as < AstTypePtr > (), yystack_[3].value.as < std::string > (), yystack_[1].value.as < std::vector<std::tuple<AstTypePtr, std::string>> > ());
  }
#line 989 "frontend/generated/parser.cpp"
    break;

  case 19: // FuncDef: Type IDENTIFIER '(' FuncParamList ')' $@2 BlockStmt
#line 149 "frontend/parser.y"
              {
    driver.quit_function();
  }
#line 997 "frontend/generated/parser.cpp"
    break;

  case 20: // ExprStmt: Expr ';'
#line 155 "frontend/parser.y"
             {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_expr_stmt(yystack_[1].value.as < AstExprPtr > ());
  }
#line 1005 "frontend/generated/parser.cpp"
    break;

  case 21: // DeclStmt: Type DefList ';'
#line 161 "frontend/parser.y"
                     {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_decl_stmt(driver.curr_decl_scope, false, yystack_[1].value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ());
  }
#line 1013 "frontend/generated/parser.cpp"
    break;

  case 22: // DeclStmt: CONST Type DefList ';'
#line 164 "frontend/parser.y"
                           {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_decl_stmt(driver.curr_decl_scope, true, yystack_[1].value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ());
    driver.is_curr_decl_const = false;
  }
#line 1022 "frontend/generated/parser.cpp"
    break;

  case 23: // DefList: DefList ',' Def
#line 171 "frontend/parser.y"
                    {
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > () = yystack_[2].value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ();
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ().push_back(yystack_[0].value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > ());

    auto symbol_entry = frontend::ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      yystack_[0].value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > ()
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
#line 1038 "frontend/generated/parser.cpp"
    break;

  case 24: // DefList: Def
#line 182 "frontend/parser.y"
        {
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>>> > ().push_back(yystack_[0].value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > ());

    auto symbol_entry = frontend::ast::create_symbol_entry_from_decl_def(
      driver.curr_decl_scope,
      driver.is_curr_decl_const,
      yystack_[0].value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > ()
    );
    driver.curr_symtable->add_symbol_entry(symbol_entry);
  }
#line 1053 "frontend/generated/parser.cpp"
    break;

  case 25: // Def: IDENTIFIER
#line 195 "frontend/parser.y"
               {
    yylhs.value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > () = std::make_tuple(driver.curr_decl_type, yystack_[0].value.as < std::string > (), std::nullopt);
  }
#line 1061 "frontend/generated/parser.cpp"
    break;

  case 26: // Def: IDENTIFIER ArrayIndices
#line 198 "frontend/parser.y"
                            {
    yylhs.value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > () = std::make_tuple(yystack_[0].value.as < AstTypePtr > (), yystack_[1].value.as < std::string > (), std::nullopt);
  }
#line 1069 "frontend/generated/parser.cpp"
    break;

  case 27: // Def: IDENTIFIER '=' InitVal
#line 201 "frontend/parser.y"
                           {
    auto init_val = yystack_[0].value.as < AstExprPtr > ();
    if (init_val->is_initializer_list()) {
      std::get<frontend::ast::expr::InitializerList>(init_val->kind)
        .set_type(driver.curr_decl_type);
    }
    yylhs.value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > () = std::make_tuple(
      driver.curr_decl_type, 
      yystack_[2].value.as < std::string > (), 
      std::make_optional(init_val)
    );
  }
#line 1086 "frontend/generated/parser.cpp"
    break;

  case 28: // Def: IDENTIFIER ArrayIndices '=' InitVal
#line 213 "frontend/parser.y"
                                        {
    auto init_val = yystack_[0].value.as < AstExprPtr > ();
    if (init_val->is_initializer_list()) {
      std::get<frontend::ast::expr::InitializerList>(init_val->kind)
        .set_type(yystack_[2].value.as < AstTypePtr > ());
    }
    yylhs.value.as < std::tuple<AstTypePtr, std::string, std::optional<AstExprPtr>> > () = std::make_tuple(yystack_[2].value.as < AstTypePtr > (), yystack_[3].value.as < std::string > (), std::make_optional(init_val));
  }
#line 1099 "frontend/generated/parser.cpp"
    break;

  case 29: // ArrayIndices: '[' Expr ']'
#line 224 "frontend/parser.y"
                 {
    // Get the corresponding array type from the `curr_decl_type`.
    auto maybe_type = frontend::create_array_type_from_expr(
      driver.curr_decl_type, std::make_optional(yystack_[1].value.as < AstExprPtr > ()));
    if (!maybe_type.has_value()) {
      std::cerr << yystack_[1].location << ":" 
                << "Array size must be const expression." << std::endl;
      YYABORT;
    }
    yylhs.value.as < AstTypePtr > () = maybe_type.value();
  }
#line 1115 "frontend/generated/parser.cpp"
    break;

  case 30: // ArrayIndices: ArrayIndices '[' Expr ']'
#line 235 "frontend/parser.y"
                              {
    auto maybe_type = frontend::create_array_type_from_expr(
      yystack_[3].value.as < AstTypePtr > (), std::make_optional(yystack_[1].value.as < AstExprPtr > ()));
    if (!maybe_type.has_value()) {
      std::cerr << yystack_[1].location << ":" 
                << "Array size must be const expression." << std::endl;
      YYABORT;
    }
    yylhs.value.as < AstTypePtr > () = maybe_type.value();
  }
#line 1130 "frontend/generated/parser.cpp"
    break;

  case 31: // InitVal: Expr
#line 248 "frontend/parser.y"
         {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1138 "frontend/generated/parser.cpp"
    break;

  case 32: // InitVal: '{' '}'
#line 251 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_initializer_list_expr({});
  }
#line 1146 "frontend/generated/parser.cpp"
    break;

  case 33: // InitVal: '{' InitValList '}'
#line 254 "frontend/parser.y"
                        {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_initializer_list_expr(yystack_[1].value.as < std::vector<AstExprPtr> > ());
  }
#line 1154 "frontend/generated/parser.cpp"
    break;

  case 34: // InitValList: InitVal
#line 260 "frontend/parser.y"
            {
    yylhs.value.as < std::vector<AstExprPtr> > ().push_back(yystack_[0].value.as < AstExprPtr > ());
  }
#line 1162 "frontend/generated/parser.cpp"
    break;

  case 35: // InitValList: InitValList ',' InitVal
#line 263 "frontend/parser.y"
                            {
    yylhs.value.as < std::vector<AstExprPtr> > () = yystack_[2].value.as < std::vector<AstExprPtr> > ();
    yylhs.value.as < std::vector<AstExprPtr> > ().push_back(yystack_[0].value.as < AstExprPtr > ());
  }
#line 1171 "frontend/generated/parser.cpp"
    break;

  case 36: // IfStmt: IF '(' Cond ')' Stmt
#line 270 "frontend/parser.y"
                                    {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_if_stmt(yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstStmtPtr > (), std::nullopt);
  }
#line 1179 "frontend/generated/parser.cpp"
    break;

  case 37: // IfStmt: IF '(' Cond ')' Stmt ELSE Stmt
#line 273 "frontend/parser.y"
                                   {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_if_stmt(yystack_[4].value.as < AstExprPtr > (), yystack_[2].value.as < AstStmtPtr > (), std::make_optional(yystack_[0].value.as < AstStmtPtr > ()));
  }
#line 1187 "frontend/generated/parser.cpp"
    break;

  case 38: // WhileStmt: WHILE '(' Cond ')' Stmt
#line 279 "frontend/parser.y"
                            {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_while_stmt(yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstStmtPtr > ());
  }
#line 1195 "frontend/generated/parser.cpp"
    break;

  case 39: // ReturnStmt: RETURN Expr ';'
#line 285 "frontend/parser.y"
                    {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_return_stmt(std::make_optional(yystack_[1].value.as < AstExprPtr > ()));
  }
#line 1203 "frontend/generated/parser.cpp"
    break;

  case 40: // ReturnStmt: RETURN ';'
#line 288 "frontend/parser.y"
               {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_return_stmt(std::nullopt);
  }
#line 1211 "frontend/generated/parser.cpp"
    break;

  case 41: // ContinueStmt: CONTINUE ';'
#line 294 "frontend/parser.y"
                 {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_continue_stmt();
  }
#line 1219 "frontend/generated/parser.cpp"
    break;

  case 42: // BreakStmt: BREAK ';'
#line 300 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_break_stmt();
  }
#line 1227 "frontend/generated/parser.cpp"
    break;

  case 43: // $@3: %empty
#line 306 "frontend/parser.y"
        {
    driver.add_block();
  }
#line 1235 "frontend/generated/parser.cpp"
    break;

  case 44: // BlockStmt: '{' $@3 Stmts '}'
#line 308 "frontend/parser.y"
              {
    yylhs.value.as < AstStmtPtr > () = driver.curr_block;
    driver.quit_block();
  }
#line 1244 "frontend/generated/parser.cpp"
    break;

  case 45: // BlockStmt: '{' '}'
#line 312 "frontend/parser.y"
            {
    // Just ignore.
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_blank_stmt();
  }
#line 1253 "frontend/generated/parser.cpp"
    break;

  case 46: // BlankStmt: ';'
#line 319 "frontend/parser.y"
        {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_blank_stmt();
  }
#line 1261 "frontend/generated/parser.cpp"
    break;

  case 47: // Expr: AddExpr
#line 326 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1269 "frontend/generated/parser.cpp"
    break;

  case 48: // Cond: LOrExpr
#line 332 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1277 "frontend/generated/parser.cpp"
    break;

  case 49: // PrimaryExpr: '(' Expr ')'
#line 338 "frontend/parser.y"
                 {
    yylhs.value.as < AstExprPtr > () = yystack_[1].value.as < AstExprPtr > ();
  }
#line 1285 "frontend/generated/parser.cpp"
    break;

  case 50: // PrimaryExpr: LVal
#line 341 "frontend/parser.y"
         {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1293 "frontend/generated/parser.cpp"
    break;

  case 51: // PrimaryExpr: INTEGER
#line 344 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_constant_expr(yystack_[0].value.as < AstComptimeValue > ());
  }
#line 1301 "frontend/generated/parser.cpp"
    break;

  case 52: // PrimaryExpr: FLOATING
#line 347 "frontend/parser.y"
             {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_constant_expr(yystack_[0].value.as < AstComptimeValue > ());
  }
#line 1309 "frontend/generated/parser.cpp"
    break;

  case 53: // LVal: IDENTIFIER
#line 353 "frontend/parser.y"
               {
    auto maybe_symbol_entry = driver.curr_symtable->lookup(yystack_[0].value.as < std::string > ());
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << yystack_[0].location << ":" << "Undefined identifier: " + yystack_[0].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_identifier_expr(maybe_symbol_entry.value());
  }
#line 1322 "frontend/generated/parser.cpp"
    break;

  case 54: // LVal: LVal '[' Expr ']'
#line 361 "frontend/parser.y"
                      {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Index, yystack_[3].value.as < AstExprPtr > (), yystack_[1].value.as < AstExprPtr > (), driver
    );
  }
#line 1332 "frontend/generated/parser.cpp"
    break;

  case 55: // AssignStmt: LVal '=' Expr ';'
#line 369 "frontend/parser.y"
                      {
    yylhs.value.as < AstStmtPtr > () = frontend::ast::create_assign_stmt(yystack_[3].value.as < AstExprPtr > (), yystack_[1].value.as < AstExprPtr > ());
  }
#line 1340 "frontend/generated/parser.cpp"
    break;

  case 56: // UnaryExpr: PrimaryExpr
#line 375 "frontend/parser.y"
                {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1348 "frontend/generated/parser.cpp"
    break;

  case 57: // UnaryExpr: '+' UnaryExpr
#line 378 "frontend/parser.y"
                  {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Pos, yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1357 "frontend/generated/parser.cpp"
    break;

  case 58: // UnaryExpr: '-' UnaryExpr
#line 382 "frontend/parser.y"
                  {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_unary_expr(
      frontend::UnaryOp::Neg, yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1366 "frontend/generated/parser.cpp"
    break;

  case 59: // UnaryExpr: '!' UnaryExpr
#line 386 "frontend/parser.y"
                  {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_unary_expr(
      frontend::UnaryOp::LogicalNot, yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1375 "frontend/generated/parser.cpp"
    break;

  case 60: // UnaryExpr: IDENTIFIER '(' FuncArgList ')'
#line 390 "frontend/parser.y"
                                   {
    auto maybe_symbol_entry = driver.compunit.symtable->lookup(yystack_[3].value.as < std::string > ());
    if (!maybe_symbol_entry.has_value()) {
      std::cerr << yystack_[3].location << ":" << "Undefined identifier: " + yystack_[3].value.as < std::string > ();
      YYABORT;
    }
    auto maybe_func_type = maybe_symbol_entry.value()->type;
    if (
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << yystack_[3].location << ":" << "Not a function: " + yystack_[3].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_call_expr(
      maybe_symbol_entry.value(), 
      yystack_[1].value.as < std::vector<AstExprPtr> > (), 
      driver
    );
  }
#line 1399 "frontend/generated/parser.cpp"
    break;

  case 61: // UnaryExpr: IDENTIFIER '(' ')'
#line 409 "frontend/parser.y"
                       {
    if (yystack_[2].value.as < std::string > () == "starttime" || yystack_[2].value.as < std::string > () == "stoptime") {
      auto maybe_symbol_entry = driver.compunit.symtable->lookup("_sysy_" + yystack_[2].value.as < std::string > ());
      int lineno = yystack_[2].location.end.line;
      auto lineno_expr = frontend::ast::create_constant_expr(
        frontend::create_comptime_value(lineno, frontend::create_int_type()));
      yylhs.value.as < AstExprPtr > () = frontend::ast::create_call_expr(
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
      !std::holds_alternative<frontend::type::Function>(maybe_func_type->kind)
    ) {
      std::cerr << yystack_[2].location << ":" << "Not a function: " + yystack_[2].value.as < std::string > ();
      YYABORT;
    }
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_call_expr(
      maybe_symbol_entry.value(), 
      {}, 
      driver
    );
  }
#line 1435 "frontend/generated/parser.cpp"
    break;

  case 62: // MulExpr: UnaryExpr
#line 443 "frontend/parser.y"
              {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1443 "frontend/generated/parser.cpp"
    break;

  case 63: // MulExpr: MulExpr '*' UnaryExpr
#line 446 "frontend/parser.y"
                          {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mul, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1452 "frontend/generated/parser.cpp"
    break;

  case 64: // MulExpr: MulExpr '/' UnaryExpr
#line 450 "frontend/parser.y"
                          {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Div, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1461 "frontend/generated/parser.cpp"
    break;

  case 65: // MulExpr: MulExpr '%' UnaryExpr
#line 454 "frontend/parser.y"
                          {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Mod, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1470 "frontend/generated/parser.cpp"
    break;

  case 66: // AddExpr: MulExpr
#line 461 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1478 "frontend/generated/parser.cpp"
    break;

  case 67: // AddExpr: AddExpr '+' MulExpr
#line 464 "frontend/parser.y"
                        {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Add, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1487 "frontend/generated/parser.cpp"
    break;

  case 68: // AddExpr: AddExpr '-' MulExpr
#line 468 "frontend/parser.y"
                        {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Sub, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1496 "frontend/generated/parser.cpp"
    break;

  case 69: // RelExpr: AddExpr
#line 475 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1504 "frontend/generated/parser.cpp"
    break;

  case 70: // RelExpr: RelExpr '<' AddExpr
#line 478 "frontend/parser.y"
                        {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Lt, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1513 "frontend/generated/parser.cpp"
    break;

  case 71: // RelExpr: RelExpr '>' AddExpr
#line 482 "frontend/parser.y"
                        {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Gt, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1522 "frontend/generated/parser.cpp"
    break;

  case 72: // RelExpr: RelExpr LE AddExpr
#line 486 "frontend/parser.y"
                       {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Le, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1531 "frontend/generated/parser.cpp"
    break;

  case 73: // RelExpr: RelExpr GE AddExpr
#line 490 "frontend/parser.y"
                       {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ge, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1540 "frontend/generated/parser.cpp"
    break;

  case 74: // EqExpr: RelExpr
#line 497 "frontend/parser.y"
            {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1548 "frontend/generated/parser.cpp"
    break;

  case 75: // EqExpr: EqExpr EQ RelExpr
#line 500 "frontend/parser.y"
                      {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Eq, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1557 "frontend/generated/parser.cpp"
    break;

  case 76: // EqExpr: EqExpr NE RelExpr
#line 504 "frontend/parser.y"
                      {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::Ne, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1566 "frontend/generated/parser.cpp"
    break;

  case 77: // LAndExpr: EqExpr
#line 511 "frontend/parser.y"
           {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1574 "frontend/generated/parser.cpp"
    break;

  case 78: // LAndExpr: LAndExpr LAND EqExpr
#line 514 "frontend/parser.y"
                         {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalAnd, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1583 "frontend/generated/parser.cpp"
    break;

  case 79: // LOrExpr: LAndExpr
#line 521 "frontend/parser.y"
             {
    yylhs.value.as < AstExprPtr > () = yystack_[0].value.as < AstExprPtr > ();
  }
#line 1591 "frontend/generated/parser.cpp"
    break;

  case 80: // LOrExpr: LOrExpr LOR LAndExpr
#line 524 "frontend/parser.y"
                         {
    yylhs.value.as < AstExprPtr > () = frontend::ast::create_binary_expr(
      frontend::BinaryOp::LogicalOr, yystack_[2].value.as < AstExprPtr > (), yystack_[0].value.as < AstExprPtr > (), driver);
  }
#line 1600 "frontend/generated/parser.cpp"
    break;

  case 81: // FuncArgList: Expr
#line 531 "frontend/parser.y"
         {
    yylhs.value.as < std::vector<AstExprPtr> > ().push_back(yystack_[0].value.as < AstExprPtr > ());
  }
#line 1608 "frontend/generated/parser.cpp"
    break;

  case 82: // FuncArgList: FuncArgList ',' Expr
#line 534 "frontend/parser.y"
                         {
    yylhs.value.as < std::vector<AstExprPtr> > () = yystack_[2].value.as < std::vector<AstExprPtr> > ();
    yylhs.value.as < std::vector<AstExprPtr> > ().push_back(yystack_[0].value.as < AstExprPtr > ());
  }
#line 1617 "frontend/generated/parser.cpp"
    break;

  case 83: // FuncParamList: FuncParam
#line 541 "frontend/parser.y"
              {
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string>> > ().push_back(yystack_[0].value.as < std::tuple<AstTypePtr, std::string> > ());
  }
#line 1625 "frontend/generated/parser.cpp"
    break;

  case 84: // FuncParamList: FuncParamList ',' FuncParam
#line 544 "frontend/parser.y"
                                {
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string>> > () = yystack_[2].value.as < std::vector<std::tuple<AstTypePtr, std::string>> > ();
    yylhs.value.as < std::vector<std::tuple<AstTypePtr, std::string>> > ().push_back(yystack_[0].value.as < std::tuple<AstTypePtr, std::string> > ());
  }
#line 1634 "frontend/generated/parser.cpp"
    break;

  case 85: // FuncParam: Type IDENTIFIER
#line 551 "frontend/parser.y"
                    {
    yylhs.value.as < std::tuple<AstTypePtr, std::string> > () = std::make_tuple(yystack_[1].value.as < AstTypePtr > (), yystack_[0].value.as < std::string > ());
  }
#line 1642 "frontend/generated/parser.cpp"
    break;

  case 86: // FuncParam: FuncParam '[' Expr ']'
#line 554 "frontend/parser.y"
                           {
    auto maybe_type = frontend::create_array_type_from_expr(
      std::get<0>(yystack_[3].value.as < std::tuple<AstTypePtr, std::string> > ()), std::make_optional(yystack_[1].value.as < AstExprPtr > ()));

    if (!maybe_type.has_value()) {
      std::cerr << yystack_[1].location << ":" << "Array size must be const expression." << std::endl;
      YYABORT;
    }

    auto type = maybe_type.value();

    yylhs.value.as < std::tuple<AstTypePtr, std::string> > () = std::make_tuple(type, std::get<1>(yystack_[3].value.as < std::tuple<AstTypePtr, std::string> > ()));
  }
#line 1660 "frontend/generated/parser.cpp"
    break;

  case 87: // FuncParam: FuncParam '[' ']'
#line 567 "frontend/parser.y"
                      {
    auto type = frontend::create_array_type_from_expr(
      std::get<0>(yystack_[2].value.as < std::tuple<AstTypePtr, std::string> > ()), std::nullopt).value();
    yylhs.value.as < std::tuple<AstTypePtr, std::string> > () = std::make_tuple(type, std::get<1>(yystack_[2].value.as < std::tuple<AstTypePtr, std::string> > ()));
  }
#line 1670 "frontend/generated/parser.cpp"
    break;

  case 88: // Type: INT
#line 575 "frontend/parser.y"
        {
    yylhs.value.as < AstTypePtr > () = frontend::create_int_type();
    driver.curr_decl_type = yylhs.value.as < AstTypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? frontend::Scope::Global 
                                                     : frontend::Scope::Local;

  }
#line 1682 "frontend/generated/parser.cpp"
    break;

  case 89: // Type: FLOAT
#line 582 "frontend/parser.y"
          {
    yylhs.value.as < AstTypePtr > () = frontend::create_float_type();
    driver.curr_decl_type = yylhs.value.as < AstTypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? frontend::Scope::Global 
                                                     : frontend::Scope::Local;
  }
#line 1693 "frontend/generated/parser.cpp"
    break;

  case 90: // Type: VOID
#line 588 "frontend/parser.y"
         {
    yylhs.value.as < AstTypePtr > () = frontend::create_void_type();
    driver.curr_decl_type = yylhs.value.as < AstTypePtr > ();
    driver.curr_decl_scope = driver.is_curr_global() ? frontend::Scope::Global 
                                                     : frontend::Scope::Local;
  }
#line 1704 "frontend/generated/parser.cpp"
    break;


#line 1708 "frontend/generated/parser.cpp"

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
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
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
  parser::symbol_name (symbol_kind_type yysymbol)
  {
    return yytnamerr_ (yytname_[yysymbol]);
  }



  // parser::context.
  parser::context::context (const parser& yyparser, const symbol_type& yyla)
    : yyparser_ (yyparser)
    , yyla_ (yyla)
  {}

  int
  parser::context::expected_tokens (symbol_kind_type yyarg[], int yyargn) const
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
  parser::yy_syntax_error_arguments_ (const context& yyctx,
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
  parser::yysyntax_error_ (const context& yyctx) const
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


  const short parser::yypact_ninf_ = -129;

  const signed char parser::yytable_ninf_ = -1;

  const short
  parser::yypact_[] =
  {
     172,   206,   206,   206,    14,   206,  -129,    -2,  -129,  -129,
       6,    48,    36,    62,    87,    52,  -129,  -129,  -129,    49,
     172,  -129,  -129,  -129,  -129,  -129,  -129,  -129,  -129,  -129,
    -129,  -129,    96,  -129,    26,  -129,  -129,    61,    39,    74,
     106,  -129,  -129,    97,  -129,   172,  -129,    81,   206,   206,
    -129,   101,  -129,  -129,   103,  -129,  -129,  -129,   206,   206,
     206,   206,   206,   206,   206,    72,    78,  -129,  -129,   138,
    -129,  -129,    32,   121,    39,    -6,    35,   111,   116,   123,
    -129,    92,   100,   122,   137,  -129,  -129,  -129,    61,    61,
      99,   206,    -5,   113,   103,  -129,  -129,  -129,   206,   172,
     206,   206,   206,   206,   206,   206,   206,   206,   172,  -129,
    -129,  -129,    12,  -129,  -129,   140,  -129,    38,   145,   132,
      99,   206,  -129,  -129,   149,    39,    39,    39,    39,    -6,
      -6,    35,   111,  -129,  -129,  -129,     8,  -129,   147,  -129,
      52,   133,  -129,  -129,   151,   172,  -129,    99,  -129,   147,
     145,  -129,   156,  -129,  -129,  -129,  -129,  -129
  };

  const signed char
  parser::yydefact_[] =
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
       0,     0,    85,    28,     0,     0,    33,     0,    17,     0,
      84,    87,     0,    30,    37,    35,    19,    86
  };

  const short
  parser::yypgoto_[] =
  {
    -129,  -129,   134,   -17,  -129,  -129,  -129,  -129,  -129,   124,
      86,  -129,  -106,  -129,  -129,  -129,  -129,  -129,  -129,  -128,
    -129,  -129,    -1,   135,  -129,     0,  -129,     3,    66,   -30,
      27,    75,    79,  -129,  -129,  -129,    42,   -14
  };

  const unsigned char
  parser::yydefgoto_[] =
  {
       0,    19,    20,    21,    22,   138,   149,    23,    24,    66,
      67,    93,   113,   136,    25,    26,    27,    28,    29,    30,
      45,    31,    32,    73,    33,    40,    35,    36,    37,    38,
      75,    76,    77,    78,    72,   117,   118,    39
  };

  const unsigned char
  parser::yytable_[] =
  {
      34,    54,    43,    56,    41,    42,   135,   116,    46,    47,
     148,    51,   100,   101,   143,     1,     2,    48,    74,    74,
      34,   156,   146,     3,   147,   112,   134,     5,    44,    16,
      17,    18,   102,   103,    58,    59,     7,     8,     9,     1,
       2,   155,    63,    64,    97,    34,    71,     3,    98,    55,
     139,     5,    56,    50,   140,   104,   105,    83,    84,    49,
       7,     8,     9,    85,    86,    87,    60,    61,    62,    34,
     125,   126,   127,   128,    74,    74,    74,    74,   119,    52,
      90,    91,   124,    92,     1,     2,    16,    17,    18,   114,
     115,   133,     3,    70,    94,    95,     5,   123,    65,    34,
      90,    91,     1,     2,    53,     7,     8,     9,    34,    68,
       3,   114,   112,    57,     5,    59,    94,   109,    80,   114,
     144,   120,   121,     7,     8,     9,   119,    81,   154,    88,
      89,   129,   130,    99,   106,   108,     1,     2,   107,   110,
     152,     1,     2,   151,     3,    34,   114,   111,     5,     3,
     137,     4,    96,     5,   141,     6,   142,     7,     8,     9,
       4,   153,     7,     8,     9,    10,   157,    11,    12,    13,
      14,    15,    16,    17,    18,     1,     2,   145,    82,    69,
     122,   131,   150,     3,    79,     4,   132,     5,     0,     6,
       0,     0,     0,     0,     0,     0,     7,     8,     9,    10,
       0,    11,    12,    13,    14,    15,    16,    17,    18,     1,
       2,     0,     0,     0,     0,     0,     0,     3,     0,     0,
       0,     5,     0,     0,     0,     0,     0,     0,     0,     0,
       7,     8,     9
  };

  const short
  parser::yycheck_[] =
  {
       0,    15,     3,    20,     1,     2,   112,    12,     5,    11,
     138,    12,    18,    19,   120,     3,     4,    11,    48,    49,
      20,   149,    14,    11,    16,    13,    14,    15,    14,    34,
      35,    36,    38,    39,     8,     9,    24,    25,    26,     3,
       4,   147,     3,     4,    12,    45,    47,    11,    16,     0,
      12,    15,    69,    17,    16,    20,    21,    58,    59,    11,
      24,    25,    26,    60,    61,    62,     5,     6,     7,    69,
     100,   101,   102,   103,   104,   105,   106,   107,    92,    17,
       8,     9,    99,    11,     3,     4,    34,    35,    36,    90,
      91,   108,    11,    12,    16,    17,    15,    98,    24,    99,
       8,     9,     3,     4,    17,    24,    25,    26,   108,    12,
      11,   112,    13,    17,    15,     9,    16,    17,    17,   120,
     121,     8,     9,    24,    25,    26,   140,    24,   145,    63,
      64,   104,   105,    12,    23,    12,     3,     4,    22,    17,
     141,     3,     4,    10,    11,   145,   147,    10,    15,    11,
      10,    13,    14,    15,     9,    17,    24,    24,    25,    26,
      13,    10,    24,    25,    26,    27,    10,    29,    30,    31,
      32,    33,    34,    35,    36,     3,     4,    28,    54,    45,
      94,   106,   140,    11,    49,    13,   107,    15,    -1,    17,
      -1,    -1,    -1,    -1,    -1,    -1,    24,    25,    26,    27,
      -1,    29,    30,    31,    32,    33,    34,    35,    36,     3,
       4,    -1,    -1,    -1,    -1,    -1,    -1,    11,    -1,    -1,
      -1,    15,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      24,    25,    26
  };

  const signed char
  parser::yystos_[] =
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
      16,     9,    24,    52,    62,    28,    14,    16,    59,    46,
      76,    10,    62,    10,    43,    52,    59,    10
  };

  const signed char
  parser::yyr1_[] =
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
  parser::yyr2_[] =
  {
       0,     2,     1,     1,     2,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     6,     0,     7,
       2,     3,     4,     3,     1,     1,     2,     3,     4,     3,
       4,     1,     2,     3,     1,     3,     5,     7,     5,     3,
       2,     2,     2,     0,     4,     2,     1,     1,     1,     3,
       1,     1,     1,     1,     4,     4,     1,     2,     2,     2,
       4,     3,     1,     3,     3,     3,     1,     3,     3,     1,
       3,     3,     3,     3,     1,     3,     3,     1,     3,     1,
       3,     1,     3,     1,     3,     2,     4,     3,     1,     1,
       1
  };


#if YYDEBUG || 1
  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a YYNTOKENS, nonterminals.
  const char*
  const parser::yytname_[] =
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
  parser::yyrline_[] =
  {
       0,    89,    89,    93,    98,   106,   109,   112,   115,   118,
     121,   124,   127,   130,   133,   136,   142,   142,   147,   147,
     155,   161,   164,   171,   182,   195,   198,   201,   213,   224,
     235,   248,   251,   254,   260,   263,   270,   273,   279,   285,
     288,   294,   300,   306,   306,   312,   319,   326,   332,   338,
     341,   344,   347,   353,   361,   369,   375,   378,   382,   386,
     390,   409,   443,   446,   450,   454,   461,   464,   468,   475,
     478,   482,   486,   490,   497,   500,   504,   511,   514,   521,
     524,   531,   534,   541,   544,   551,   554,   567,   575,   582,
     588
  };

  void
  parser::yy_stack_print_ () const
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
  parser::yy_reduce_print_ (int yyrule) const
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


} // yy
#line 2301 "frontend/generated/parser.cpp"

#line 596 "frontend/parser.y"


void yy::parser::error (const location_type& loc, const std::string& msg) {
  std::cerr << loc << ": " << msg << std::endl;
}

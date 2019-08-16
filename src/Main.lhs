\documentclass{article}
\usepackage{minted}
\usepackage{amssymb}
\usepackage{amsmath}
\usepackage{stmaryrd}
\usepackage{comment}
\newcommand{\D}{\ensuremath{\mathcal{D}} }
\newcommand{\V}{\ensuremath{\mathcal{V}} }
\newcommand{\E}{\ensuremath{\mathcal{E}} }
\newcommand{\Z}{\ensuremath{\mathbb{Z}} }
\newcommand{\B}{\ensuremath{\mathbb{B}} }
\newcommand{\AST}{\texttt{AST} }
\newcommand{\EXPR}{\texttt{EXPR} }
\newcommand{\NAME}{\texttt{NAME} }
\newcommand{\tinyfort}{\texttt{tinyfort}}
% \newcommand{\sema}[1]{\ensuremath{\left{[} #1 \right{]}}}
\newcommand{\sema}[1]{\ensuremath{\llbracket #1 \rrbracket}}
\newenvironment{code}{\VerbatimEnvironment \begin{minted}{haskell}}{\end{minted}}
\begin{document}
\section{Introduction}
\tinyfort is a strongly typed, fortran inspired imperative programming
language. It wishes to be as fast as C for common workloads, while providing
much more safety with a strong type system, array bounds checks, and
no undefined behavior.

\begin{comment}
\begin{code}
{-# LANGUAGE RecordWildCards #-}
{-# LANGUAGE TupleSections #-}
{-# LANGUAGE DeriveFunctor #-}
import Data.Char
import Data.Monoid
import Control.Monad
import Control.Monad.Fail
import Control.Applicative

data Parser s a = Parser { runParser :: s -> [(a, s)] } deriving(Functor)

instance Applicative (Parser s) where
  pure = return
  (<*>) = ap

instance Alternative (Parser s) where
    empty = Parser $ const []
    pa <|> pb = Parser $ \s -> 
                  case runParser pa s of
                    [] -> runParser pb s
                    xs -> xs

instance Monad (Parser s) where
   return a = Parser $ \s -> [(a, s)]
   pa >>= a2pb = Parser $ \s -> do
                   (a, s') <- runParser pa s
                   runParser (a2pb a) s' 
instance MonadFail (Parser s) where
    -- | failing pattern match will create a failing parser
    fail s = empty

-- | parser that fails
failp :: Parser s a
failp = empty

digitp :: Parser String Char
digitp = Parser $ \i -> 
  case i of
    [] -> []
    (x:xs) -> if isDigit x then [(x, xs)] else []

letterp :: Parser String Char
letterp = Parser $ \i ->
    case i of
      [] -> []
      (x:xs) -> if isLetter x then [(x, xs)] else []
                    

-- | parser for string
strp :: String -> Parser String String
strp s = Parser $ \input -> 
  let (begin, end) = splitAt (length s) input
  in if begin == s then [(begin, end)] else []

-- | parser that matches a string and returns a value
matchp :: String -> a -> Parser String a
matchp s a = do
    _ <- strp s
    return a


-- | parser that returns this value by default without taking any input
defaultp :: a -> Parser s a
defaultp a = Parser $ \s -> [(a, s)]

isCharWhitespace :: Char -> Bool
isCharWhitespace c = c == ' ' || c == '\n' || c == '\t' 

-- | eat whitespace.
eatWhitespace :: Parser String ()
eatWhitespace = Parser $ \s -> [((), dropWhile isCharWhitespace s)]

-- | take till whitespace. parser fails if no string is read.
-- TODO: maybe we should not fail as a parser
readTillWhitespace :: Parser String String
readTillWhitespace = Parser $ \s -> 
    let begin = takeWhile (not . isCharWhitespace) s 
    in if null begin
       then []
       else [(begin, dropWhile (not . isCharWhitespace) s)]

-- | Take an element from te parser
takep :: Parser [a] a
takep = Parser $ \as -> 
  case as of
    [] -> []
    (a:as) -> [(a, as)]

-- | parser that expects a given value
expectp :: Eq a => a -> Parser [a] a
expectp a = Parser $ \as ->
  case as of
    [] -> []
    (a':as) -> if a == a' then [(a, as)] else []

-- | zero or more
starp :: Parser s a -> Parser s [a]
starp p = liftA2 (:) p (starp p) <|> defaultp []

-- | one or more
plusp :: Parser s a -> Parser s [a]
plusp p = liftA2 (:) p (starp p)

-- | bracket parser with left and right tokens
bracketp :: Eq a => a -> Parser [a] o -> a -> Parser [a] o
bracketp l p r = do
 expectp l
 o <- p
 expectp r
 return $ o

maybep :: Parser s a -> Parser s (Maybe a)
maybep p = (Just <$> p) <|> defaultp Nothing

\end{code}
\end{comment}

\section{Syntax}

We freely use regular expression syntax to make our syntax definitions shorter.
Standard encodings of regular expression operators within CFGs is assumed.

\begin{itemize}
\item \texttt{*}: Zero or more occurrences.
\item \texttt{+}: One or more occurrences.
\item \texttt{?}: Exactly zero or one occurrence.
\end{itemize}

metavariables are written \texttt{verbatim}. Exact string literals to be matched
are enclosed by double quotes, \texttt{"like so"}.

\begin{minted}{text}
toplevel := (functiondecl | declarestmt)*

functiondecl := "def" name "(" args ")" "->" type block

block := "{" stmt+ "}"

stmt := returnstmt | declarestmt | assignstmt | ifstmt | ifelsestmt | whilestmt
returnstmt := "return" expr ";"
declarestmt := "let" "mut"? name ":" type
| "let" "mut"? name ":" type "=" expr ";"
assignstmt := "set" lval ":=" expr ";"
ifstmt := "if" expr block
whilestmt := "while" expr block


lval := name | lval "[" indeces "]"
indeces := expr "," indeces | expr


expr := expr2 == expr2 | expr2 <= expr2 | expr2 >= expr2 | expr2 != expr2 | expr2
expr2 := expr3 + expr3 | expr3 - expr3 | expr3
expr3 := expr4 * expr4 | expr4 / expr4
expr4 := integer | float | bool | lval | "(" expr ")" | "!" expr

basetype := "int" | "bool" | "file"
type := basetype "[" indeces "]" | basetype

integer := "-"?[0-9]+
bool := "true" | "false"
string := """ letter* """
\end{minted}

\begin{code}
data Binop = Eq | Neq | Leq | Geq | Add | Sub | Mul | Div deriving(Eq, Show, Ord)
type Name = String
data LVal = LValName Name | LValIndex Name [Expr] deriving(Eq, Show, Ord)
data Expr = EInt Integer 
            | EBool Bool 
            | EBinop Expr Binop Expr 
            | ELVal LVal 
            | ECall Name [Expr] deriving(Eq, Show, Ord)

data Tok = TIf | TElse | TWhile | TName Name | TInt Integer | TBool Bool | TLCurly | TRCurly | TComma
  | TLRoundBrack | TRRoundBrack | TLSquareBrack | TRSquareBrack | TArrow | TSemicolon | TBinop Binop | TSet | TLet | TColonEq | TColon  | TEq
  | TTyInt | TTyBool | TripleDash | TLCurlyBrack | TRCurlyBrack deriving(Eq, Show,Ord)

data Stmt = StmtLet LVal Type (Maybe Expr) | StmtAssign LVal Expr | StmtBlock [Stmt] | StmtIf Expr Stmt (Maybe Stmt) | StmtWhile Expr Stmt

data Type = TyInt | TyBool | TyArr Type [Expr]


-- | Tokenize the raw string into tokens
tokenizep :: Parser String [Tok]
tokenizep = starp (eatWhitespace >> tokenp) where
  tokenp = (matchp "if" TIf) <|>
           (matchp "else" TElse) <|>
           (matchp "while" TWhile) <|>
           (TInt <$> integerp) <|>
           (TBool <$> boolp) <|>
           (TName <$> namep) <|>
           (matchp "+" (TBinop Add)) <|>
           (matchp "-" (TBinop Sub)) <|>
           (matchp "*" (TBinop Mul)) <|>
           (matchp "/" (TBinop Div)) <|>
           (matchp "==" (TBinop Eq)) <|>
           (matchp "(" TLRoundBrack) <|>
           (matchp ")" TRRoundBrack) <|>
           (matchp "[" TLSquareBrack) <|>
           (matchp "]" TRSquareBrack) <|>
           (matchp "," TComma) <|>
           (matchp "set" TSet) <|>
           (matchp "let" TLet) <|>
           (matchp ":=" TColonEq) <|>
           (matchp ":" TColon) <|>
           (matchp "=" TEq) <|>
           (matchp "int" TTyInt) <|>
           (matchp "bool" TTyBool)  <|>
           (matchp "---" TripleDash)

-- | Parser for names
namep :: Parser String Name
namep = do
 c <- letterp
 cs <- starp (letterp <|> digitp)
 return (c:cs)

-- | Parser for integers
integerp :: Parser String Integer
integerp = do
  negative <- (strp "-") <|> (defaultp "")
  ints <- plusp digitp
  return $ read (negative <> ints)

-- | Parser for booleans
boolp :: Parser String Bool
boolp = do
        s <- readTillWhitespace
        case s of
          "true" -> return True
          "false" -> return False
          x -> failp
-- | Parser for binary operations
binopp :: Binop -> Parser [Tok] Binop
binopp op = 
  Parser $ \ts -> 
    case ts of
      (TBinop op':ts') -> if op == op' then [(op, ts')] else []
      _ -> []

-- | Parser for
exprp :: Parser [Tok] Expr
exprp = 
  (do
    el <- expr2p
    op <- binopp Eq <|> binopp Leq <|> binopp Geq <|> binopp Neq
    er <- expr2p
    return (EBinop el op er)) <|> expr2p


expr2p :: Parser [Tok] Expr
expr2p = 
  (do
    el <- expr3p
    op <- binopp Add <|> binopp Sub
    er <- expr3p
    return (EBinop el op er)) <|> expr3p

expr3p :: Parser [Tok] Expr
expr3p = 
  (do
    el <- expr4p
    op <- binopp Mul <|> binopp Div
    er <- expr4p
    return (EBinop el op er)) <|> expr4p


expr4p = callp <|>
  (do
    (TInt i) <- takep
    return (EInt i)) <|>
  (do
    (TBool b) <- takep
    return (EBool b)) <|>
    (bracketp TLRoundBrack exprp TRRoundBrack) <|> 
    (ELVal <$> lvalp) 


callp :: Parser [Tok] Expr
callp = do
    (TName name) <- takep
    params <- bracketp TLRoundBrack indecesp  TRRoundBrack
    return (ECall name params)

indecesp :: Parser [Tok] [Expr]
indecesp = do
  e <- exprp
  es <- (expectp TComma >> indecesp) <|> (defaultp [])
  return (e:es)

lvalp :: Parser [Tok] LVal
lvalp = 
  (do
    (TName name) <- takep
    indeces <- bracketp TLSquareBrack  indecesp TRSquareBrack
    return (LValIndex name indeces)
  ) <|> 
  (do
    (TName name) <- takep
    return (LValName name))

basetypep :: Parser [Tok] Type
basetypep = 
  (expectp TTyInt >> return TyInt) <|>
  (expectp TTyBool >> return TyBool)

typep :: Parser [Tok] Type
typep = 
  (do
    bt <- basetypep
    indeces <- bracketp TLSquareBrack indecesp TRSquareBrack
    return (TyArr bt indeces)) <|>
  basetypep

-- | declaration
letp :: Parser [Tok] Stmt
letp = do
    expectp TSet
    lval <- lvalp
    expectp TColon
    ty <- typep
    expectp TEq
    rhs <- Just <$> exprp <|> (expectp TripleDash >> pure Nothing)
    return $ StmtLet lval ty rhs

assignp :: Parser [Tok] Stmt
assignp = do
    lval <- lvalp
    expectp TEq
    rhs <- exprp
    return $ StmtAssign lval rhs

blockp :: Parser [Tok] Stmt
blockp = StmtBlock <$> bracketp TLCurlyBrack (starp stmtp) TRCurlyBrack

ifp :: Parser [Tok] Stmt
ifp = do
    expectp TIf
    cond <- exprp
    then_ <- stmtp
    else_ <- maybep $ do
              expectp TElse
              stmtp
    return (StmtIf cond then_ else_)
    

whilep :: Parser [Tok] Stmt
whilep = do
  expectp TWhile
  cond <- exprp
  body <- stmtp
  return $ StmtWhile cond body

stmtp :: Parser [Tok] Stmt
stmtp = blockp <|> letp <|> ifp <|> whilep <|> assignp 


\end{code}

\section{Semantics}
Let \AST be the set of all syntax trees as defined by the grammar in the
syntax section. $\EXPR \subset \AST$ is the set of all expression trees
as defined by the grammar.

Let \NAME be the set of all names which occur in the program.
Let \V be the set of values, which is union of integers \Z and booleans \B.
Let ${\E \equiv \NAME \rightarrow \V}$ be the set of environments.
Then, \D is the domain on which we define our semantics, which is
defined as
${\D \equiv (\NAME \rightarrow (\E \rightarrow \E)}$. 
That is, we map
each toplevel function or binding in the program to a transformer, which when
given the initial environment, produces the final environment.

The denotation of different parts of the AST is given by a semantic function
${\sema{~} : \AST \rightarrow \D}$, which is defined compositionally, as shown
below.

By abuse of notation, we overload $\sema{~}$ on expressions as well,
to define ${\sema{~} : \EXPR \rightarrow (\E \rightarrow \V)}$, which given an
expression, maps it to a function which takes
the current state of the environment and returns the value of the expression.

\begin{center}
\begin{align*}
\sema{~} &: \EXPR \rightarrow (\E \rightarrow \V) \\
\sema{\texttt{integer}}(env) &= integer \\
\sema{\texttt{true}}(env) &= true \\
\sema{\texttt{false}}(env) &= false  \\
%
\sema{e_1 \star e_2}(env) &= \sema{e_1}(env) \star \sema{e_2}(env) \quad \star \in \{ \texttt{+}, \texttt{-}, \texttt{*}, \texttt{/}, \texttt{==}, \texttt{/=} \} \\
\sema{\texttt{name}}(env) &= env(\texttt{name}) \\
\sema{\text{lval[index]}}(env) &= env(\texttt{lval})(env(\texttt{index})) \\
\end{align*}
\end{center}

\section{Semantic checks}
\begin{itemize}
\item Checking that variables are defined before they are used.
\item Checking that the variables' type at declaration is respected at every mutation of the variable.
\item Checking that expressions are well-typed.
\item Checking that array sizes can be computed at compile time.
\item Check array bounds at compile time and error report those
  which are obviously wrong. 
\end{itemize}
\section{Example programs}

\begin{code}
main :: IO ()
main = do 
  let parses = runParser tokenizep "a[1, b[1] + c] + a(1, 2, 3)"
  print $ parses
  print $ parses >>= \(toks, _) -> runParser exprp toks
\end{code}
\end{document}

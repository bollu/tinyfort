\documentclass{article}
\usepackage{minted}
\usepackage{amssymb}
\usepackage{amsmath}
\usepackage{stmaryrd}
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
\begin{document}
\section{Introduction}
\tinyfort is a strongly typed, fortran inspired imperative programming
language. It wishes to be as fast as C for common workloads, while providing
much more safety with a strong type system, array bounds checks, and
no undefined behavior.

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


lval := name | lval "[" expr "]"

expr := expr2 == expr2 | expr2 <= expr2 | expr2 >= expr2 | expr2 != expr2 | expr2
expr2 := expr3 + expr3 | expr3 - expr3 | expr3
expr3 := expr4 * expr4 | expr4 / expr4
expr4 := integer | float | bool | lval | "(" expr ")" | expr "as" type

type := "int" | "bool" | "char" | type "[" expr "]"

integer := [0-9]+
float := integer "." integer
bool := "true" | "false"
string := """ letter* """
\end{minted}

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
\end{document}


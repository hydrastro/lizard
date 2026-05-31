; -*- lisp -*-
; lib/compile/to-c.lisp — the Phase 9 native compiler.
;
; (program->c forms) translates a list of top-level Scheme forms into a C
; source string that, compiled against liblizard.a and the lzrt runtime,
; reuses lizard's value representation, primitives, numeric tower, printer and
; conservative GC.  Because data operations delegate to the very same C
; primitives the interpreter uses, a compiled program is observably identical
; to the interpreted one over the supported subset.
;
; Strategy: closure conversion (free variables captured into a heap array the
; GC scans) + lexical addressing (parameters -> av[i], captured vars -> fv[j],
; globals/primitives -> lzrt_global_ref) + trampolined proper tail calls
; (tail position emits lzrt_tailcall with a heap argv; lzrt_apply drives the
; trampoline).  Codegen is C89-clean: every temporary is declared at the top of
; its function and control flow uses nested blocks that contain only
; statements.
;
; Supported subset (v1): integer/rational/real/boolean/string/symbol/() and
; quoted list literals; variable reference; if; begin; lambda (fixed and rest
; args) with full closures; application (with proper tail calls); top-level
; define (mutual recursion works via late global binding); set! on
; locals/parameters; and derived forms let, let*, cond, and, or, when, unless
; (desugared).  Deliberately NOT in v1: user-defined macros, call/cc,
; nested/internal define and letrec (use top-level define or let), and
; set! on a variable captured by another closure (no boxing yet).

; ----------------------------------------------------------------------------
;  small list helpers (cadr/caddr etc. are not primitives here)
; ----------------------------------------------------------------------------
(define (cadr x) (car (cdr x)))
(define (caddr x) (car (cdr (cdr x))))
(define (cddr x) (cdr (cdr x)))
(define (cdddr x) (cdr (cdr (cdr x))))
(define (last-pair l) (if (pair? (cdr l)) (last-pair (cdr l)) l))

; ----------------------------------------------------------------------------
;  compiler state: a name counter and the accumulated C function definitions
; ----------------------------------------------------------------------------
(define *ctr* (atom 0))
(define *fns* (atom ""))

(define (gensym prefix)
  (swap! *ctr* (lambda (n) (+ n 1)))
  (string-append prefix (number->string (deref *ctr*))))

(define (emit-fn! text)
  (swap! *fns* (lambda (s) (string-append s text))))

(define (reset-compiler!)
  (swap! *ctr* (lambda (n) 0))
  (swap! *fns* (lambda (s) "")))

; ----------------------------------------------------------------------------
;  C string-literal escaping (no char primitives, so iterate via substring)
; ----------------------------------------------------------------------------
(define (c-escape-1 ch)
  (cond ((string=? ch "\"") "\\\"")
        ((string=? ch "\\") "\\\\")
        ((string=? ch "\n") "\\n")
        ((string=? ch "\t") "\\t")
        (else ch)))

(define (c-string-literal s)
  (define n (string-length s))
  (define (loop i acc)
    (if (>= i n)
        (string-append "\"" acc "\"")
        (loop (+ i 1)
              (string-append acc (c-escape-1 (substring s i (+ i 1)))))))
  (loop 0 ""))

; ----------------------------------------------------------------------------
;  literals -> C expression (pure expressions, no statements)
; ----------------------------------------------------------------------------
(define (rational->c d)
  ; (/ num den) at runtime reconstructs the exact rational
  (string-append "lzrt_apply(lzrt_global_ref(\"/\"),lzrt_argv2("
                 "lzrt_int_str(\"" (number->string (numerator d)) "\"),"
                 "lzrt_int_str(\"" (number->string (denominator d)) "\")),2)"))

(define (real->c d)
  ; exact->inexact of the exact rational equal to the IEEE double => same double
  (let ((q (inexact->exact d)))
    (string-append "lzrt_apply(lzrt_global_ref(\"exact->inexact\"),lzrt_argv1("
                   "lzrt_apply(lzrt_global_ref(\"/\"),lzrt_argv2("
                   "lzrt_int_str(\"" (number->string (numerator q)) "\"),"
                   "lzrt_int_str(\"" (number->string (denominator q)) "\")),2)),1)")))

(define (lit->c d)
  (cond ((exact-integer? d)
         (string-append "lzrt_int_str(\"" (number->string d) "\")"))
        ((and (exact? d) (not (exact-integer? d))) (rational->c d))
        ((inexact? d) (real->c d))
        ((string? d) (string-append "lzrt_string(" (c-string-literal d) ")"))
        ((symbol? d) (string-append "lzrt_symbol(\"" (symbol->string d) "\")"))
        ((null? d) "lzrt_nil()")
        ((equal? d #t) "lzrt_bool(1)")
        ((equal? d #f) "lzrt_bool(0)")
        (else (raise (string-append "lit->c: cannot emit literal")))))

; quoted datum -> C expression that rebuilds it (lists become cons chains)
(define (quote->c d)
  (cond ((pair? d)
         (string-append "lzrt_cons(" (quote->c (car d)) ","
                        (quote->c (cdr d)) ")"))
        (else (lit->c d))))

; ----------------------------------------------------------------------------
;  free-variable analysis over s-expressions
;    (free-vars expr bound) -> list of names referenced free in expr
; ----------------------------------------------------------------------------
(define (mem? x lst) (if (member x lst) #t #f))

(define (union a b)
  (cond ((null? a) b)
        ((mem? (car a) b) (union (cdr a) b))
        (else (cons (car a) (union (cdr a) b)))))

(define (remove-all items lst)
  (cond ((null? lst) '())
        ((mem? (car lst) items) (remove-all items (cdr lst)))
        (else (cons (car lst) (remove-all items (cdr lst))))))

; collect symbols of a (possibly dotted) param spec into a flat list
(define (param-names spec)
  (cond ((symbol? spec) (list spec))
        ((null? spec) '())
        ((pair? spec) (cons (car spec) (param-names (cdr spec))))
        (else '())))

(define (fv-list exprs bound)
  (cond ((null? exprs) '())
        (else (union (free-vars (car exprs) bound)
                     (fv-list (cdr exprs) bound)))))

(define (free-vars expr bound)
  (cond
    ((symbol? expr) (if (mem? expr bound) '() (list expr)))
    ((not (pair? expr)) '())               ; self-quoting literal
    ((equal? (car expr) 'quote) '())
    ((equal? (car expr) 'lambda)
     (free-vars-seq (cddr expr) (union (param-names (cadr expr)) bound)))
    ((equal? (car expr) 'define) '())      ; only top-level defines in v1
    ((equal? (car expr) 'let)
     (let ((binds (cadr expr)))
       (union (fv-list (map cadr binds) bound)
              (free-vars-seq (cddr expr)
                             (union (map car binds) bound)))))
    ((equal? (car expr) 'let*)
     (free-vars (let*->let expr) bound))
    ((equal? (car expr) 'set!)
     (union (if (mem? (cadr expr) bound) '() (list (cadr expr)))
            (free-vars (caddr expr) bound)))
    (else (fv-list expr bound))))          ; if/begin/cond/and/or/app: all subforms

(define (free-vars-seq exprs bound) (fv-list exprs bound))

; ----------------------------------------------------------------------------
;  desugaring of derived forms
; ----------------------------------------------------------------------------
(define (let*->let expr)
  (let ((binds (cadr expr)) (body (cddr expr)))
    (cond ((null? binds) (cons 'begin body))
          ((null? (cdr binds))
           (cons 'let (cons binds body)))
          (else
           (list 'let (list (car binds)) (let*->let (cons 'let* (cons (cdr binds) body))))))))

; (cond (t e...) ... (else e...)) -> nested if
(define (cond->if clauses)
  (cond ((null? clauses) (list 'quote #f))
        ((equal? (car (car clauses)) 'else)
         (cons 'begin (cdr (car clauses))))
        (else
         (list 'if (car (car clauses))
               (cons 'begin (cdr (car clauses)))
               (cond->if (cdr clauses))))))

; ----------------------------------------------------------------------------
;  code generation.  (comp expr env tail?) -> (list decls stmts result-cexpr)
;    decls : C declarations (placed at the enclosing function's top)
;    stmts : C statements to run before the value is available
;    result-cexpr : C expression denoting the value
;  env : alist mapping a name -> its C access expression ("av[0]" / "fv[1]")
; ----------------------------------------------------------------------------
(define (R decls stmts result) (list decls stmts result))
(define (R-decls r) (car r))
(define (R-stmts r) (cadr r))
(define (R-result r) (caddr r))

(define (comp expr env tail)
  (cond
    ((symbol? expr)
     (let ((b (assoc expr env)))
       (if b (R "" "" (cdr b))
           (R "" "" (string-append "lzrt_global_ref(\"" (symbol->string expr) "\")")))))
    ((not (pair? expr)) (R "" "" (lit->c expr)))
    ((equal? (car expr) 'quote) (R "" "" (quote->c (cadr expr))))
    ((equal? (car expr) 'if) (comp-if expr env tail))
    ((equal? (car expr) 'begin) (comp-seq (cdr expr) env tail))
    ((equal? (car expr) 'lambda) (comp-lambda expr env))
    ((equal? (car expr) 'set!) (comp-set expr env))
    ((equal? (car expr) 'let) (comp (let->lambda expr) env tail))
    ((equal? (car expr) 'let*) (comp (let*->let expr) env tail))
    ((equal? (car expr) 'cond) (comp (cond->if (cdr expr)) env tail))
    ((equal? (car expr) 'and) (comp-and (cdr expr) env tail))
    ((equal? (car expr) 'or) (comp-or (cdr expr) env tail))
    ((equal? (car expr) 'when)
     (comp (list 'if (cadr expr) (cons 'begin (cddr expr)) (list 'quote #f)) env tail))
    ((equal? (car expr) 'unless)
     (comp (list 'if (cadr expr) (list 'quote #f) (cons 'begin (cddr expr))) env tail))
    ((equal? (car expr) 'define)
     (raise "compile: internal define is not supported in v1; use let or top-level define"))
    (else (comp-app expr env tail))))

(define (let->lambda expr)
  (let ((binds (cadr expr)) (body (cddr expr)))
    (cons (cons 'lambda (cons (map car binds) body))
          (map cadr binds))))

(define (comp-if expr env tail)
  (let ((c (comp (cadr expr) env #f))) (let ((t (comp (caddr expr) env tail))) (let ((e (if (null? (cdddr expr))
                (R "" "" "lzrt_bool(0)")
                (comp (car (cdddr expr)) env tail)))) (let ((res (gensym "t"))) (R (string-append (R-decls c) (R-decls t) (R-decls e)
                      "  lizard_ast_node_t *" res ";\n")
       (string-append
        (R-stmts c)
        "  if (lzrt_truthy(" (R-result c) ")) {\n"
        (R-stmts t) "    " res " = " (R-result t) ";\n"
        "  } else {\n"
        (R-stmts e) "    " res " = " (R-result e) ";\n"
        "  }\n")
       res))))))

; a sequence: all but the last are for effect; the last carries tail-ness
(define (comp-seq exprs env tail)
  (cond ((null? exprs) (R "" "" "lzrt_nil()"))
        ((null? (cdr exprs)) (comp (car exprs) env tail))
        (else
         (let ((h (comp (car exprs) env #f))
               (rest (comp-seq (cdr exprs) env tail)))
           (R (string-append (R-decls h) (R-decls rest))
              (string-append (R-stmts h) "  " (R-result h) ";\n" (R-stmts rest))
              (R-result rest))))))

(define (comp-and exprs env tail)
  (cond ((null? exprs) (R "" "" "lzrt_bool(1)"))
        ((null? (cdr exprs)) (comp (car exprs) env tail))
        (else (comp (list 'if (car exprs) (cons 'and (cdr exprs)) (list 'quote #f))
                    env tail))))

; or must preserve the first true VALUE, so bind it to a temp
(define (comp-or exprs env tail)
  (cond ((null? exprs) (R "" "" "lzrt_bool(0)"))
        ((null? (cdr exprs)) (comp (car exprs) env tail))
        (else
         (let ((h (comp (car exprs) env #f))) (let ((rest (comp-or (cdr exprs) env tail))) (let ((res (gensym "t"))) (R (string-append (R-decls h) (R-decls rest)
                             "  lizard_ast_node_t *" res ";\n")
              (string-append
               (R-stmts h)
               "  if (lzrt_truthy(" (R-result h) ")) {\n"
               "    " res " = " (R-result h) ";\n"
               "  } else {\n"
               (R-stmts rest) "    " res " = " (R-result rest) ";\n"
               "  }\n")
              res)))))))

(define (comp-set expr env)
  (let ((b (assoc (cadr expr) env))
        (v (comp (caddr expr) env #f)))
    (if (not b)
        (raise "compile: set! on a non-local (global) variable is not supported in v1")
        (R (R-decls v)
           (string-append (R-stmts v) "  " (cdr b) " = " (R-result v) ";\n")
           "lzrt_bool(1)"))))

; build the argv element-assignment statements; returns (list decls stmts arrname)
(define (args-each items i env arrname dacc sacc)
  (if (null? items)
      (list dacc sacc)
      (let ((c (comp (car items) env #f)))
        (args-each (cdr items) (+ i 1) env arrname
                   (string-append dacc (R-decls c))
                   (string-append sacc (R-stmts c)
                                  "  " arrname "[" (number->string i) "] = "
                                  (R-result c) ";\n")))))

(define (comp-args args env arrname heap?)
  (let ((n (length args))) (let ((parts (args-each args 0 env arrname "" ""))) (let ((decls0 (car parts))) (let ((stmts0 (cadr parts))) (let ((decl-arr
          (if heap?
              (string-append "  lizard_ast_node_t **" arrname ";\n")
              (if (= n 0) ""
                  (string-append "  lizard_ast_node_t *" arrname "["
                                 (number->string n) "];\n"))))) (let ((alloc (if heap?
                    (string-append "  " arrname " = lzrt_argv("
                                   (number->string n) ");\n")
                    ""))) (list (string-append decls0 decl-arr)
          (string-append alloc stmts0)
          arrname))))))))

(define (comp-app expr env tail)
  (let ((fc (comp (car expr) env #f))) (let ((fname (gensym "f"))) (let ((args (cdr expr))) (let ((n (length args))) (let ((arr (gensym (if tail "ta" "aa")))) (let ((ac (comp-args args env arr tail))) (if tail
        ; tail call: heap argv survives the returning frame; emit the bounce
        (R (string-append (R-decls fc) "  lizard_ast_node_t *" fname ";\n"
                          (car ac))
           (string-append (R-stmts fc) "  " fname " = " (R-result fc) ";\n"
                          (cadr ac))
           (string-append "lzrt_tailcall(" fname ", " arr ", "
                          (number->string n) ")"))
        ; non-tail: stack argv, drive the trampoline to a value
        (let ((res (gensym "t")))
          (R (string-append (R-decls fc) "  lizard_ast_node_t *" fname ";\n"
                            (car ac) "  lizard_ast_node_t *" res ";\n")
             (string-append (R-stmts fc) "  " fname " = " (R-result fc) ";\n"
                            (cadr ac)
                            "  " res " = lzrt_apply(" fname ", "
                            (if (= n 0) "0" arr) ", " (number->string n) ");\n")
             res))))))))))

; ----------------------------------------------------------------------------
;  lambda: closure-convert, emit a top-level C function, return its constructor
; ----------------------------------------------------------------------------
(define (pp-walk s fixed)
  (cond ((null? s) (list (reverse fixed) #f (length fixed)))
        ((symbol? s)
         (let ((f (reverse (cons s fixed)))) (list f #t (length f))))
        (else (pp-walk (cdr s) (cons (car s) fixed)))))

(define (parse-params spec)
  ; -> (list fixed-names variadic? arity)
  (cond ((symbol? spec) (list (list spec) #t 1))
        (else (pp-walk spec '()))))

(define (index-of x lst)
  (define (loop i l)
    (cond ((null? l) -1)
          ((equal? (car l) x) i)
          (else (loop (+ i 1) (cdr l)))))
  (loop 0 lst))

; build the body environment: params -> av[i], captures -> fv[j]
(define (bind-params names i acc)
  (if (null? names) acc
      (bind-params (cdr names) (+ i 1)
                   (cons (cons (car names)
                               (string-append "av[" (number->string i) "]"))
                         acc))))
(define (bind-caps names j acc)
  (if (null? names) acc
      (bind-caps (cdr names) (+ j 1)
                 (cons (cons (car names)
                             (string-append "fv[" (number->string j) "]"))
                       acc))))
(define (assign-caps names i env cap-arr sacc)
  (if (null? names) sacc
      (let ((b (assoc (car names) env)))
        (assign-caps (cdr names) (+ i 1) env cap-arr
                     (string-append sacc "  " cap-arr "["
                                    (number->string i) "] = " (cdr b) ";\n")))))

(define (comp-lambda expr env)
  (let ((pinfo (parse-params (cadr expr))))
   (let ((pnames (car pinfo)))
    (let ((variadic (cadr pinfo)))
     (let ((arity (caddr pinfo)))
      (let ((body (cddr expr)))
       (let ((bodyfree (free-vars-seq body pnames)))
        (let ((caps (filter (lambda (nm) (assoc nm env)) bodyfree)))
         (let ((fnname (gensym "fn_")))
          (let ((env2 (bind-caps caps 0 (bind-params pnames 0 (quote ())))))
           (let ((cbody (comp (cons (quote begin) body) env2 #t)))
            (let ((cap-arr (gensym "cv")))
             (let ((ncaps (length caps)))
              (emit-fn!
               (string-append
                "static lizard_ast_node_t *" fnname
                "(lizard_ast_node_t **fv, lizard_ast_node_t **av, long ac) {\n"
                (R-decls cbody)
                "  (void)fv; (void)av; (void)ac;\n"
                (R-stmts cbody)
                "  return " (R-result cbody) ";\n}\n\n"))
              (if (= ncaps 0)
                  (R "" ""
                     (string-append "lzrt_closure(" fnname ", 0, 0, "
                                    (number->string arity) ", "
                                    (if variadic "1" "0") ")"))
                  (R (string-append "  lizard_ast_node_t *" cap-arr "["
                                    (number->string ncaps) "];\n")
                     (assign-caps caps 0 env cap-arr "")
                     (string-append "lzrt_closure(" fnname ", " cap-arr ", "
                                    (number->string ncaps) ", "
                                    (number->string arity) ", "
                                    (if variadic "1" "0") ")"))))))))))))))))

; ----------------------------------------------------------------------------
;  top level
; ----------------------------------------------------------------------------
(define (define-name+value form)
  ; (define (f a b) body...) | (define x e) -> (list name value-expr)
  (let ((target (cadr form)))
    (if (pair? target)
        (list (car target)
              (cons 'lambda (cons (cdr target) (cddr form))))
        (list target (caddr form)))))

(define (comp-toplevel-form form)
  ; -> (list decls stmts)   (statements run inside main, after lzrt_init)
  (if (and (pair? form) (equal? (car form) 'define))
      (let ((nv (define-name+value form))) (let ((nm (car nv))) (let ((c (comp (cadr nv) '() #f))) (list (R-decls c)
              (string-append (R-stmts c)
                             "  lzrt_define_global(\"" (symbol->string nm)
                             "\", " (R-result c) ");\n")))))
      (let ((c (comp form '() #f)))
        (list (R-decls c)
              (string-append (R-stmts c) "  " (R-result c) ";\n")))))

(define c-preamble
  (string-append
   "/* Generated by lib/compile/to-c.lisp — the lizard native compiler. */\n"
   "#include \"lzrt.h\"\n"
   "#include <stddef.h>\n\n"
   "/* small fixed-size argv builders used by literal emission */\n"
   "static lizard_ast_node_t **lzrt_argv1(lizard_ast_node_t *a) {\n"
   "  lizard_ast_node_t **v = lzrt_argv(1); v[0] = a; return v; }\n"
   "static lizard_ast_node_t **lzrt_argv2(lizard_ast_node_t *a,"
   " lizard_ast_node_t *b) {\n"
   "  lizard_ast_node_t **v = lzrt_argv(2); v[0] = a; v[1] = b; return v; }\n\n"))

(define (tl-collect fs dacc sacc)
  (if (null? fs)
      (list dacc sacc)
      (let ((tf (comp-toplevel-form (car fs))))
        (tl-collect (cdr fs)
                    (string-append dacc (car tf))
                    (string-append sacc (cadr tf))))))

(define (program->c forms)
  (reset-compiler!)
  (let ((parts (tl-collect forms "" ""))) (let ((decls (car parts))) (let ((stmts (cadr parts))) (string-append
     c-preamble
     (deref *fns*)
     "int main(void) {\n"
     decls
     "  lzrt_init();\n"
     stmts
     "  return 0;\n}\n")))))

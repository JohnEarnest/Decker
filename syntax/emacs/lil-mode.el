; lil-mode for emacs
; (c) 2024 tangentstorm.
; available for use under the MIT license
(require 'generic-x)
(defun wds (xs)
  (string-join
   (list "\\<\\("
         (string-join (mapcar 'symbol-name xs) "\\|")
         "\\)\\>") ""))
(define-generic-mode 'lil-mode
  '("#") ; comments
  '("while" "end" "if" "each" "else" "elseif" "on" "do" "in"
    "send" "local"
    "extract" "select" "update" "insert"
    "where" "by" "orderby" "asc" "desc" "group" "from")
  `(("^#.*" . 'font-lock-comment-face)
    ("[-!+*,&|/%^<>=~@:]" . 'font-lock-builtin-face)
    (,(wds '(floor cos sin tan exp ln sqrt
             sum prod raze min max
             typeof count first last
             range keys list flip
             rows cols table mag heading unit
             split fuse like dict take window
             drop limit in unless join cross
             parse format fill))
     . 'font-lock-builtin-face)
    (,(wds '(show panic print play go transition brush
                  sleep array image sound newdeck eval random
                  readcsv writecsv readxml writexml
                  alert read write input error dir
                  path exit shell import))
     . 'font-lock-function-name-face)
    ("\\<[0-9]+\\>" . 'font-lock-constant-face))
  '("\\.lil$")             ; which files
  nil                      ; other functions
  "mode for editing lil files")

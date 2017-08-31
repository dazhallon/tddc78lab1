(TeX-add-style-hook
 "report"
 (lambda ()
   (TeX-add-to-alist 'LaTeX-provided-class-options
                     '(("article" "twocolumn")))
   (TeX-add-to-alist 'LaTeX-provided-package-options
                     '(("inputenc" "utf8")))
   (TeX-run-style-hooks
    "latex2e"
    "article"
    "art10"
    "inputenc"
    "amsmath"
    "amsthm"
    "amssymb"
    "subcaption"
    "graphicx"
    "diagbox"
    "listings"
    "color")
   (TeX-add-symbols
    "listingsfont"
    "listingsfontinline")
   (LaTeX-add-labels
    "sec:code-description"
    "sec:blur-filter"
    "sec:threshold-filter"
    "sec:results"
    "sec:comp-results"
    "tab:1"
    "tab:2"
    "sec:filter-results"
    "fig:thres"
    "sec:code")))


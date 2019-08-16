.PHONY: openpaper

openpaper: paper/paper.pdf
	xdg-open paper/paper.pdf

paper/paper.tex: src/Main.lhs
	mkdir -p paper
	pandoc src/Main.lhs -o paper/paper.tex

paper/paper.pdf: paper/paper.tex
	cd paper && pdflatex  -shell-escape paper.tex 






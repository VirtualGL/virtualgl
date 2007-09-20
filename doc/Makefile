all: index.html

.PHONY: pdf
pdf: virtualgl.pdf
tex: virtualgl.tex

SRC := $(wildcard *.txt *.css *.rb *.png)
index.html: $(SRC)
virtualgl.tex: $(SRC)

index.html:
	deplate -c config.rb -m html-obfuscate-email virtualgl.txt -o $@

virtualgl.pdf: virtualgl.tex
	pdflatex $<

virtualgl.tex:
	$(RM) -f deplate.sty
	deplate -c config.rb -f latex --pdf virtualgl.txt

.PHONY: clean
clean:
	$(RM) -f index.html virtualgl.tex virtualgl.pdf virtualgl.aux \
		virtualgl.toc virtualgl.out virtualgl.lot virtualgl.lof



all: pdf

pdf: clean
	pandoc -o Report.pdf Report.md \
	-V geometry:margin=1.75cm \
	-V urlcolor=cyan \
	--number-sections \
	--variable=documentclass:report \
	--toc -V fontsize=12pt



clean:
	rm -f *.pdf

#
# TransFig makefile
#

all: cliconf.tex 

# translation into epic

cliconf.tex: cliconf.fig Makefile
	fig2dev -L epic cliconf.fig > cliconf.tex
clean::
	rm -f cliconf.tex

cliconf.eps: cliconf.fig Makefile
	fig2dev -L eps cliconf.fig > cliconf.eps
clean::
	rm -f cliconf.eps


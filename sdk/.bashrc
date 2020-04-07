cd ~

PS1="\u@\h (\w)$ "
export PS1

if [ -f ~/.bash_extras ]; then
	source ~/.bash_extras
fi
